package main

import (
	"bufio"
	"fmt"
	"math"
	"net"
	"net/rpc"
	"os"
	"sort"
	"strconv"
	"strings"
	"sync"
	"time"
)

type Resp struct{}

type M map[string]string

// var queue []M
var seq_list map[float64][]float64
var id_list map[float64][]int
var numNode int
var checksum float64

var connGroup [9]*rpc.Client

type Packet struct {
	From        int     //send from which node
	SequenceNum float64 //the sequence number of the packet
	Message     string  //message of the packet
	Priority    float64 //priority
	Final       bool    //final priority if Final = true
	Replyfrom   int
}

//datastruture stored in queue
type store struct {
	SequenceNum float64
	Priority    float64
	Count       [9]bool
	Deliverable bool
	Message     string
	From        int
}

var processid int //self id

var alivenode int

type Q []store

var (
	mtx            sync.Mutex
	total_count    int
	dataB          map[string]int
	alivecheck     []bool
	selfpriority   float64
	agreedpriority float64
	received       map[string]bool
	delivered      map[float64]bool
	queue          Q
)

func doEvery(d time.Duration, f func(time.Time)) {
	for x := range time.Tick(d) {
		f(x)
	}
}

//print Balance every 5 sec
func printBalance(t time.Time) {
	var sortedKeys []string
	mtx.Lock()
	for k, _ := range dataB {
		sortedKeys = append(sortedKeys, k)
	}
	sort.Strings(sortedKeys)
	temp := 0
	fmt.Printf("total delivered transmission now : %d\n", total_count)
	fmt.Printf("BALANCES : ")
	for _, k := range sortedKeys {
		tempy := dataB[k]
		if tempy == 0 {
			continue
		}
		temp += tempy
		fmt.Printf("%s: %d  ", k, tempy)
	}
	fmt.Println(" check sum: ", temp)
	fmt.Printf("\n")
	mtx.Unlock()
}

func (a Q) Len() int {
	return len(a)
}

func (a Q) Less(i, j int) bool {
	return a[i].Priority < a[j].Priority
}

func (a Q) Swap(i, j int) {
	a[i], a[j] = a[j], a[i]
}

//multicast the message
func multicast(result Packet) {
	// out, _ := json.Marshal(&result)
	// fmt.Printf("send: %s\n", result)
	var wg sync.WaitGroup
	// fmt.Printf("send: %s\n", result)
	for i := 1; i < 9; i++ {
		// runtime.Gosched()
		if connGroup[i] == nil {
			continue
		}
		wg.Add(1)
		go func(i int) {
			// fmt.Printf("send: %s\n", result)
			var response Packet
			err := connGroup[i].Call("Resp.Handler", result, &response)
			if err != nil {
				fmt.Printf("node %d is disconnected \n", i)
				connGroup[i] = nil
				alivecheck[i] = false
			}
			wg.Done()
		}(i)
	}
	wg.Wait()
}

func transaction(Message string) {
	s := strings.Split(Message, " ")
	// for _, val := range s {
	// 	fmt.Printf("%s ", val)
	// }
	// fmt.Println()
	total_count++
	switch s[0] {
	case "DEPOSIT":
		// fmt.Println("here is DEPO")
		To := s[1]
		tempamount := s[2]
		temps := strings.Split(tempamount, "\n")
		amount, err := strconv.Atoi(temps[0])
		val, ok := dataB[To]
		// fmt.Printf("tempamount : %s\n", tempamount)
		// fmt.Printf("convert amount : %d\n", amount)
		if err != nil {
			fmt.Println(err.Error())
		}
		if ok {
			dataB[To] = val + (amount)
			// fmt.Println("amount :", dataB[To])
		} else {
			dataB[To] = (amount)
			// fmt.Println("amount :", dataB[To])
		}

	case "TRANSFER":
		// fmt.Println("here is TRANS")
		From := s[1]
		To := s[3]
		tempamount := s[4]
		temps := strings.Split(tempamount, "\n")
		amount, _ := strconv.Atoi(temps[0])
		val, ok := dataB[From]
		if ok && val >= amount { //have the "From" acount and that acount has enough money
			dataB[From] = val - (amount) //subtruct the money from sender
			val2, ok2 := dataB[To]
			if ok2 {
				dataB[To] = val2 + (amount)
			} else {
				dataB[To] = (amount)
			}
		} else {
			fmt.Println("Illegal transactions")
		}
	}
	// printBalance()
}

//hanling the incoming channel
func (R *Resp) Handler(pkt *Packet, response *Packet) error {
	//reliable multicast
	var tempkey1 string = strconv.FormatFloat(pkt.SequenceNum, 'f', -1, 64)
	var tempkey2 string = strconv.FormatInt(int64(pkt.Replyfrom), 10)
	receivekey := tempkey1 + "+" + tempkey2 + "+" + strconv.FormatBool(pkt.Final)
	key2 := tempkey1 + "+" + tempkey2 + "+" + strconv.FormatBool(true)
	// fmt.Printf("receivekey: %s\n", receivekey)
	response.SequenceNum = 0
	mtx.Lock()
	if _, ok := received[receivekey]; ok { //this package is already received
		// fmt.Printf("receivekey: %s\n", receivekey)
		mtx.Unlock()
		return nil
	}
	//mark as received
	received[receivekey] = true
	mtx.Unlock()
	// fmt.Printf("receivekey: %s\n", receivekey)
	//when receive a package from the incomming channel, there are two possibility, other's response or request
	if pkt.From == processid { //this package is the reply from other node.
		//update the queue
		// fmt.Printf("key: %s\n", receivekey)
		mtx.Lock()
		for i, val := range queue {
			if val.SequenceNum == pkt.SequenceNum {
				queue[i].Priority = math.Max(val.Priority, pkt.Priority)
				queue[i].Count[pkt.Replyfrom] = true
				// fmt.Printf("seq: %f, count: %d\n", pkt.SequenceNum, queue[i].Count)
				tempcheck := 0
				tempalive := 0
				for j, v := range queue[i].Count {
					if alivecheck[j] == true {
						tempalive++
						if v == true {
							tempcheck++
						}

					}
				}
				// fmt.Println(tempcheck)
				if tempcheck == tempalive { //receive all response
					//multicast
					// fmt.Printf("get all reply\n")
					// fmt.Printf("pkt %f is deliverabal\n", pkt.SequenceNum)
					pkt.Final = true
					queue[i].Deliverable = true
					pkt.Priority = math.Max(val.Priority, pkt.Priority)
					received[key2] = true
					// go multicast(*pkt)
					//sort and check deliver
					sort.Sort(queue)
					go multicast(*pkt)
					//deliver
					for {
						if len(queue) > 0 {
							if queue[0].Deliverable {

								// fmt.Printf("deliver :%s with count %d\n", queue[0].Message, total_count)
								transaction(queue[0].Message)
								checksum += queue[0].SequenceNum
								// fmt.Println("checksum: ", checksum)
								queue = queue[1:] //pop the first element
								continue
							} else if queue[0].From != processid { //the sender is c
								if connGroup[queue[0].From] == nil {
									queue = queue[1:]
									continue
								}
							} else { //check if the waiting reply is crashed
								tempcheck := 0
								tempalive := 0
								for i, v := range queue[0].Count {
									if alivecheck[i] == true {
										tempalive++
										if v == true {
											tempcheck++
										}

									}
								}
								if tempalive == tempcheck {
									queue[0].Deliverable = true
									// fmt.Printf("it is deliverable now \n")
									continue
								}
							}
						}
						break
					}
				}
				break
			}
		}
		mtx.Unlock()
	} else { //this is a request package, reply back and multicast.
		// fmt.Printf("request: %s\n", receivekey)
		//check if it is the final priority
		if pkt.Final == true {
			mtx.Lock()
			go multicast(*pkt)
			agreedpriority = math.Max(agreedpriority, pkt.Priority)
			for i, val := range queue {
				if val.SequenceNum == pkt.SequenceNum {
					queue[i].Priority = pkt.Priority
					queue[i].Deliverable = true
					//sort and deliver
					sort.Sort(queue)
					//deliver
					for {
						if len(queue) > 0 {
							if queue[0].Deliverable {
								// fmt.Printf("deliver :%s with count %d\n", queue[0].Message, total_count)
								transaction(queue[0].Message)
								checksum += queue[0].SequenceNum
								// fmt.Println("checksum: ", checksum)
								queue = queue[1:] //pop the first element
								continue
							} else if queue[0].From != processid { //the sender is c
								if connGroup[queue[0].From] == nil {
									queue = queue[1:]
									continue
								}
							} else { //check if the waiting reply is crashed
								tempcheck := 0
								tempalive := 0
								for i, v := range queue[0].Count {
									if alivecheck[i] == true {
										tempalive++
										if v == true {
											tempcheck++
										}

									}
								}
								if tempalive == tempcheck {
									queue[0].Deliverable = true
									continue
								}
							}
						}
						break
					}
					break
				}
			}
			mtx.Unlock()
		} else {
			mtx.Lock()
			selfpriority = math.Max(selfpriority, agreedpriority) + 1
			reply := Packet{pkt.From, pkt.SequenceNum, pkt.Message, selfpriority, pkt.Final, processid}
			// *response =
			// reply := packet{pkt.From, pkt.SequenceNum, pkt.Message, selfpriority, pkt.Final, processid}
			//push to the queue
			var tcount [9]bool
			temps := store{pkt.SequenceNum, selfpriority, tcount, pkt.Final, pkt.Message, pkt.From}
			queue = append(queue, temps)
			mtx.Unlock()
			err := connGroup[pkt.From].Call("Resp.Handler", reply, &response) //reply back
			if err != nil {
				connGroup[pkt.From] = nil
				alivecheck[pkt.From] = false
			}
		}
	}
	return nil
}

func main() {
	arguments := os.Args
	name, _ := os.Hostname()
	numNode, _ = strconv.Atoi(arguments[1])
	alivenode = numNode
	port := ":" + arguments[2]
	fmt.Println("hostname:", name)
	host := strings.Split(strings.Split(name, ".")[0], "-")
	id := host[len(host)-1]
	fmt.Println("id:", id)
	idInt, _ := strconv.Atoi(id)
	processid = idInt
	dataB = make(map[string]int)
	seq_list = make(map[float64][]float64)
	id_list = make(map[float64][]int)
	delivered = make(map[float64]bool)
	received = make(map[string]bool)
	alivecheck = make([]bool, 9)
	total_count = 0
	Rsp := new(Resp)
	rpc.Register(Rsp)
	rpc.HandleHTTP()
	checksum = 0.0
	// connGroup = make([9]net.Conn)
	go func() {
		// s, _ := net.ResolveTCPAddr("udp", port)
		tcpAddr, err := net.ResolveTCPAddr("tcp", port)
		listener, err := net.ListenTCP("tcp", tcpAddr)
		// s, err := net.Listen("tcp", port)
		if err != nil {
			fmt.Println("listen fail")
			os.Exit(1)
		}
		// fmt.Println("listen on port: " + port)
		for {
			//fmt.Println("ready for connection")
			conn, err := listener.Accept()
			if err != nil {
				fmt.Println("acception fail")
				os.Exit(1)
			}
			go rpc.ServeConn(conn)
		}
	}()
	time.Sleep(time.Second)
	fmt.Println("after listen")
	for i := 1; i <= numNode; i++ {
		if i == idInt {
			continue
		}
		client, err := rpc.Dial("tcp", "sp20-cs425-g02-0"+strconv.Itoa(i)+".cs.illinois.edu"+port)
		fmt.Println("here")
		// err = client.Call("Arith.Multiply", args, &reply)
		if err != nil {
			fmt.Println("dial fail")
			os.Exit(1)
		}
		// defer conn.Close()
		fmt.Println("debug: ", i)
		connGroup[i] = client
		alivecheck[i] = true
		//listenreply()
	}
	fmt.Println("afer dial")
	time.Sleep(2 * time.Second)
	go doEvery(5*time.Second, printBalance)
	counter := 0
	reader := bufio.NewReader(os.Stdin)
	selfpriority = float64(idInt) / float64(10)
	//make the package to send
	for {
		text, _ := reader.ReadString('\n')
		counter++
		// digitLen := len(strconv.Itoa(numNode))
		seqnum := float64(counter) + float64(idInt)/float64(10)
		var tcount [9]bool
		tcount[idInt] = true
		mtx.Lock()
		selfpriority++
		var tempkey1 string = strconv.FormatFloat(seqnum, 'f', -1, 64)
		var tempkey2 string = strconv.FormatInt(int64(idInt), 10)
		temps := store{seqnum, selfpriority, tcount, false, text, idInt} //need fix here
		result := Packet{idInt, seqnum, text, selfpriority, false, idInt}
		//push the packet in the queue.
		queue = append(queue, temps)
		receivekey := tempkey1 + "+" + tempkey2 + "+" + strconv.FormatBool(false) //generate key for the received package
		received[receivekey] = true
		// fmt.Printf("msg send : %s \n", out)
		mtx.Unlock()
		if len(text) == 0 {
			return
		}
		go multicast(result) //outgoing channel

	}

}
