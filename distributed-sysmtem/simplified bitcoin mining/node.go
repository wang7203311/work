//usr/bin/env go run $0 "$@"; exit
package main

import (
	"bufio"
	"bytes"
	"crypto/sha256"
	"encoding/gob"
	"encoding/hex"
	"fmt"
	"log"
	"net"
	"os"
	"runtime"
	"strconv"
	"strings"
	"sync"
	"time"
	"math/rand"
)

type Block struct {
	Parent string
	Log    []string
	Puzzle string
	Height int
}
type Blockchain struct {
	genesis string
	blkmap  map[string]Block
	tip     string
	height  int
	orphan  map[string]Block
	state   map[string]int
}

var mem []string
var memState map[string]int
var receivedBlkMem map[string]Block
var finishedMem map[string]string
var nodeid string
var pendingBlock map[string]Block
var nodeList map[string]string
var connNodeList map[string]net.Conn
var string_chan chan string
var network_string_chan chan string
var request_chan chan string
var blk_chan chan Block
var blockchain Blockchain
var mtx sync.Mutex
var currentNeighbors int

const NEIGHBORS_MAX int = 20
const BLOCK_SIZE int = 2000
const TARGET string = "fced80b4ead5f7c499d232c936757e152769254ff059eba4ddad9786dd8edbcc"


func doEvery(d time.Duration, f func(time.Time)) {
	for x := range time.Tick(d) {
		f(x)
		runtime.Gosched()
	}
}
func printBalance(t time.Time) {
    sum:=0
    for _, balance := range blockchain.state {
		sum+=balance
	}
    fmt.Printf("%s neighbors:%d checksum: %d, longest chain height: %d\n",nodeid,currentNeighbors,sum,blockchain.height)
}
func checkConn(t time.Time) {
    if currentNeighbors<NEIGHBORS_MAX{
        for key, url := range nodeList {
            _,ok:=connNodeList[key]
            if !ok{
                host := strings.Split(url, ":")
                var ret []string
                ret = append(ret, "INTRODUCE", key, host[0], host[1])
                result := strings.Join(ret, " ")+"\n"
                network_string_chan <- result
            }
        }
    }
}

func updateState(blk Block) {
	for _, trans := range blk.Log {
		strlist := strings.Split(strings.Trim(trans, "\t\n"), " ")
		mtx.Lock()
		_, ok := finishedMem[strlist[2]]
		mtx.Unlock()
		if !ok {
			sender := strlist[3]
			receiver := strlist[4]
			amount, _ := strconv.Atoi(strlist[5])
			if sender == "0" {
				mtx.Lock()
				blockchain.state[receiver] += amount
				mtx.Unlock()
			} else {
				balance, ok1 := blockchain.state[sender]
				if ok1 && balance >= amount {
					mtx.Lock()
					blockchain.state[sender] -= amount
					blockchain.state[receiver] += amount
					mtx.Unlock()
				}
			}
			mtx.Lock()
			finishedMem[strlist[2]] = trans
			mtx.Unlock()
		}
	}
	//fmt.Println(nodeid,": ",blockchain.state)
}

func insert(blk Block, blkHash string) {

    if blockchain.genesis == "" {
        blockchain.genesis = blkHash
        blockchain.height += 1
        blk.Height += 1
        blockchain.blkmap[blkHash] = blk
        blockchain.tip = blkHash
        updateState(blk)
    } else {
        parent, found := blockchain.blkmap[blk.Parent]
        _, exist := blockchain.blkmap[blkHash]
        if !found || exist {
            return
        }
        h := parent.Height + 1
        if h > blockchain.height {
            blockchain.height = h
            blockchain.tip = blkHash
            if blk.Parent!="0"{updateState(parent)}
        }
        blk.Height = h
        blockchain.blkmap[blkHash] = blk
    }
}

func sortMem() int{
	changed := true
	unknown_length := len(mem) - 1
	return_length := unknown_length+1
	for changed == true {
		changed = false
		for index := 0; index < unknown_length; index++ {
			if strings.Split(mem[index], " ")[1] > strings.Split(mem[index+1], " ")[1] {
		        mtx.Lock()
				mem[index], mem[index+1] = mem[index+1], mem[index]
		        mtx.Unlock()
				changed = true
			}
		}
		unknown_length -= 1
	}
	return return_length
}

func hash(blk Block) string {
	var network bytes.Buffer        // Stand-in for a network connection
	enc := gob.NewEncoder(&network) // Will write to network.
	err := enc.Encode(blk)
	if err != nil {
		log.Fatal("encode error:", err)
	}
	sha256Bytes := sha256.Sum256(network.Bytes())
	hash := hex.EncodeToString(sha256Bytes[:])
	return hash
}

func serviceRequest(conn net.Conn) {
	for {
		request_msg := <-request_chan
		fmt.Println(request_msg)
		fmt.Fprintf(conn, "%s\n", request_msg)
		runtime.Gosched()
	}
}

func multicastBlock(newBlk Block) {
	bin_buf := new(bytes.Buffer)
	gobobj := gob.NewEncoder(bin_buf)
	gobobj.Encode(newBlk)
	for _, conn := range connNodeList {
		var ret []string
		ret = append(ret, "BLOCK", hex.EncodeToString(bin_buf.Bytes()))
		result := strings.Join(ret, " ")
		fmt.Fprintf(conn, "%s\n", result)
	}
}
func multicastTransaction(newTransaction string) {
	for _, conn := range connNodeList {
		fmt.Fprintf(conn, "%s\n", newTransaction)
	}
}
func networkFlow(conn net.Conn) {
	reader := bufio.NewReader(conn)
	for {
		msg, err := reader.ReadString('\n')
		if err == nil {
			if strings.Split(strings.Trim(msg, "\t\n"), " ")[0] == "BLOCK" {
				decoded, err1 := hex.DecodeString(strings.Split(strings.Trim(msg, "\t\n"), " ")[1])
				if err1 != nil {
					log.Fatal(err)
				}
				tmpbuff := bytes.NewBuffer(decoded)
				tmpBlock := new(Block)
				gobobj := gob.NewDecoder(tmpbuff)
				gobobj.Decode(tmpBlock)
				blk_chan <- *tmpBlock
			} else {
			    //if strings.Split(strings.Trim(msg, "\t\n"), " ")[0] == "TRANSACTION" {fmt.Println("new",msg)}
				//fmt.Println("new",msg)
				network_string_chan <- msg
			}

		} else {
			// fmt.Println(err.Error())
			conn.Close()
			t := time.Now().UnixNano()
			x := float64(t) / float64(1000000000)
			s := fmt.Sprintf("%f", x)
			mtx.Lock()
			currentNeighbors-=1
			mtx.Unlock()
			fmt.Printf("%s\n", s+" - disconnected")
			return
		}
		runtime.Gosched()
	}
}
func networkHandler(url string) {
	for {
		msg := <-network_string_chan
		//fmt.Println("new",msg)
		msg_list := strings.Split(strings.Trim(msg, "\t\n"), " ")
		head := msg_list[0]
		if head == "INTRODUCE" {
			_, ok := connNodeList[msg_list[1]]
			_,ok1 := nodeList[msg_list[1]]
			newUrl := msg_list[2] + ":" + msg_list[3]
            if !ok1{
                mtx.Lock()
                nodeList[msg_list[1]] = newUrl
                mtx.Unlock()
            }
            rand.Seed(time.Now().UnixNano())
            accept := rand.Intn(2)
			if !ok && newUrl != url && currentNeighbors<NEIGHBORS_MAX && accept==1{
                newConn, err := net.Dial("tcp", newUrl)
                if err != nil {
                    continue
                }
                mtx.Lock()
                connNodeList[msg_list[1]] = newConn
                mtx.Unlock()
                var ret []string
                ret = append(ret, "REQUEST", url, nodeid)
                result := strings.Join(ret, " ")
                fmt.Fprintf(newConn, "%s\n", result)
                    //fmt.Println(msg_list[2]+":"+msg_list[3])
			}

		} else if head == "REQUEST" {
			//fmt.Println(msg)
			request_id := msg_list[2]
			reqConn, ok := connNodeList[request_id]
			_,ok1 := nodeList[msg_list[2]]
			if !ok1{
                mtx.Lock()
                nodeList[msg_list[2]] = msg_list[1]
                mtx.Unlock()
            }
			if !ok {
				newConn, err := net.Dial("tcp", msg_list[1])
				if err != nil {
					panic(err)
				}
                mtx.Lock()
                connNodeList[request_id] = newConn
                mtx.Unlock()
				reqConn = newConn
			}
			for key, newUrl := range nodeList {
				if key != msg_list[2] {
					host := strings.Split(newUrl, ":")
					var ret []string
					ret = append(ret, "INTRODUCE", key, host[0], host[1])
					result := strings.Join(ret, " ")
					fmt.Fprintf(reqConn, "%s\n", result)
				}
			}
		} else if head == "TRANSACTION" {
			is_exist := false
			mtx.Lock()
			_, ok := finishedMem[msg_list[2]]
			mtx.Unlock()

            if !ok {
                for _, tran := range mem {
                    if strings.Split(strings.Trim(tran, "\t\n"), " ")[2] == msg_list[2] {
                        is_exist = true
                        break
                    }
                }
                if !is_exist {
                    balance,ok1:=memState[msg_list[3]]
                    amount, _ := strconv.Atoi(msg_list[5])
                    if msg_list[3]=="0" || (ok1 && balance>=amount){
                        multicastTransaction(strings.Trim(msg, "\t\n"))
                        mtx.Lock()
                        mem = append(mem, msg)
                        mtx.Unlock()
                        if msg_list[3]=="0"{
                            mtx.Lock()
                            memState[msg_list[4]] += amount
                            mtx.Unlock()
                        }else{
                            mtx.Lock()
                            memState[msg_list[3]] -= amount
                            memState[msg_list[4]] += amount
                            mtx.Unlock()
                        }
                    }
                }
            }
		}
		runtime.Gosched()
	}

}
func handler(url string) {
	for {
		msg := <-string_chan
		//fmt.Println("new",msg)
		msg_list := strings.Split(strings.Trim(msg, "\t\n"), " ")
		head := msg_list[0]
		if head == "INTRODUCE" {
			_, ok := connNodeList[msg_list[1]]
			newUrl := msg_list[2] + ":" + msg_list[3]

			if !ok && newUrl != url && currentNeighbors<NEIGHBORS_MAX{
                newConn, err := net.Dial("tcp", newUrl)
                if err != nil {
                    continue
                }
                mtx.Lock()
                nodeList[msg_list[1]] = newUrl
                connNodeList[msg_list[1]] = newConn
                mtx.Unlock()
                var ret []string
                ret = append(ret, "REQUEST", url, nodeid)
                result := strings.Join(ret, " ")
                fmt.Fprintf(newConn, "%s\n", result)
                    //fmt.Println(msg_list[2]+":"+msg_list[3])
			}

		} else if head == "TRANSACTION" {
			is_exist := false
			mtx.Lock()
			_, ok := finishedMem[msg_list[2]]
			mtx.Unlock()
            if !ok {
                for _, tran := range mem {
                    if strings.Split(strings.Trim(tran, "\t\n"), " ")[2] == msg_list[2] {
                        is_exist = true
                        break
                    }
                }
                if !is_exist {
                    balance,ok1:=memState[msg_list[3]]
                    amount, _ := strconv.Atoi(msg_list[5])
                    if msg_list[3]=="0" || (ok1 && balance>=amount){
                        multicastTransaction(strings.Trim(msg, "\t\n"))
                    }
                }
            }

		} else if head == "SOLVED" {
			fmt.Printf("%s\n", msg)
			solvedBlkHash := msg_list[1]
			solvedBlk,_ := pendingBlock[solvedBlkHash]
			solvedBlk.Puzzle = msg_list[2]
			mtx.Lock()
			delete(pendingBlock, solvedBlkHash)
			mtx.Unlock()
			newBlkHash := hash(solvedBlk)
			mtx.Lock()
			//
			mtx.Unlock()
			//fmt.Printf("block hash: %s\n", newBlkHash)
			//fmt.Printf("Neighbors number: %d\n", len(connNodeList))
			if newBlkHash < TARGET {
				mtx.Lock()
				//_, ok := blockchain.blkmap[solvedBlk.Parent]
				_, ok1 := blockchain.blkmap[newBlkHash]
				mtx.Unlock()
				if !ok1 {
					multicastBlock(solvedBlk)
				}
			}
		} else if head == "VERIFY" {
			verifiedBlkHash := msg_list[2]
			verifiedBlk := pendingBlock[verifiedBlkHash]
			verifiedBlk.Puzzle = msg_list[3]
			mtx.Lock()
			delete(pendingBlock, verifiedBlkHash)
			mtx.Unlock()
			newBlkHash := hash(verifiedBlk)
			if msg_list[1] == "OK" {
				if newBlkHash < TARGET {
					_, ok := blockchain.blkmap[verifiedBlk.Parent]
					_, ok1 := blockchain.blkmap[newBlkHash]
					if !ok1 {
						if ok {
							insert(verifiedBlk, newBlkHash)
							for key,blk:= range blockchain.orphan{
							    _, newok := blockchain.blkmap[blk.Parent]
							    _, newok1 := blockchain.blkmap[key]
							    if newok && !newok1{
                                    insert(blk, key)
                                    mtx.Lock()
                                    delete(blockchain.orphan,key)
                                    mtx.Unlock()
							    }
							}
						} else {
							blockchain.orphan[newBlkHash] = verifiedBlk
						}
					}
				}
			}
		}
		runtime.Gosched()
	}

}

func miner() {
	mining_height := 0
	for {
		if (mining_height <= blockchain.height && len(mem)>3) || mining_height == 0 {
			mining_height = blockchain.height + 1
			newBlk := Block{Height: 0}
			if blockchain.tip == "" {
				newBlk.Parent = "0"
			} else {
				newBlk.Parent = blockchain.tip
				tip_log:= blockchain.blkmap[blockchain.tip].Log
				if len(tip_log)>0{
                    last_transaction:=tip_log[len(tip_log)-1]
                    for index,tran:=range mem{
                        if strings.Trim(tran, "\t\n")==strings.Trim(last_transaction, "\t\n"){
                            mtx.Lock()
                            mem = mem[index+1:]
                            mtx.Unlock()
                            break
                        }
                    }
                }
			}

			for _,trans:=range mem{
			    if len(newBlk.Log) > BLOCK_SIZE {break}
			    //fmt.Println("mempool length:",currentLength)
			    //if len(newBlk.Log)>=currentLength{
			    //    currentLength = sortMem()+len(newBlk.Log)
			    //}
				trans_id := strings.Split(trans, " ")[1]
				mtx.Lock()
				_, ok := finishedMem[trans_id]
				mtx.Unlock()
				if !ok {
					newBlk.Log = append(newBlk.Log, trans)
				}
			}
			//fmt.Println(newBlk)
			blkhash := hash(newBlk)
			mtx.Lock()
			pendingBlock[blkhash] = newBlk
			mtx.Unlock()
			if blockchain.tip == "" {
				fmt.Printf("Genesis Block\n")
				insert(newBlk, blkhash)
			} else {
				var ret []string
				ret = append(ret, "SOLVE", blkhash)
				result := strings.Join(ret, " ")
				request_chan <- result
			}
		}
		runtime.Gosched()
	}
}
func blockHandler() {
	for {
		newBlock := <-blk_chan

		solution := newBlock.Puzzle
        solvedBlk := newBlock
        solvedBlk.Puzzle = ""
        solvedBlkHash := hash(solvedBlk)

        _, ok := receivedBlkMem[solvedBlkHash]

		if !ok {
		    multicastBlock(newBlock)
			//fmt.Println("decode block: ", newBlkHash)
			mtx.Lock()
			receivedBlkMem[solvedBlkHash] = solvedBlk
			pendingBlock[solvedBlkHash] = solvedBlk
			mtx.Unlock()

			var ret []string
			ret = append(ret, "VERIFY", solvedBlkHash, solution)
			result := strings.Join(ret, " ")
			//fmt.Printf("%s\n", result)
			request_chan <- result
		}
		runtime.Gosched()
	}
}
func main() {
	arguments := os.Args
	name, _ := os.Hostname()
	port := ":" + arguments[1]
	service_port := ":" + arguments[2]
	fmt.Println("hostname:", name)
	host := strings.Split(strings.Split(name, ".")[0], "-")
	id := host[len(host)-1]
	fmt.Println("id:", id)
	string_chan = make(chan string)
	network_string_chan = make(chan string)
	blk_chan = make(chan Block)
	request_chan = make(chan string)
	connNodeList = make(map[string]net.Conn)
	finishedMem = make(map[string]string)
	receivedBlkMem = make(map[string]Block)
	nodeList = make(map[string]string)
	pendingBlock = make(map[string]Block)
	memState = make(map[string]int)
	blockchain = Blockchain{height: 0}
	blockchain.blkmap = make(map[string]Block)
	blockchain.orphan = make(map[string]Block)
	blockchain.state = make(map[string]int)
	nodeid = id+arguments[1]
	currentNeighbors = 0
	// Tell the 'wg' WaitGroup how many threads/goroutines
	//   that are about to run concurrently.
	go func() {
		s, err := net.Listen("tcp", port)
		if err != nil {
			fmt.Println("listen fail")
			os.Exit(1)
		}
		// fmt.Println("listen on port: " + port)
		defer s.Close()
		for {
			//fmt.Println("ready for connection")
			conn, err := s.Accept()
			if err != nil {
				fmt.Println("acception fail")
				os.Exit(1)
			}
			if currentNeighbors<NEIGHBORS_MAX{
			    go networkFlow(conn)
			    mtx.Lock()
                currentNeighbors+=1
                mtx.Unlock()
			}else{conn.Close()}
		}
	}()
	go handler(name+port)
	go networkHandler(name+port)
	conn, err := net.Dial("tcp", "sp20-cs425-g02-01.cs.illinois.edu"+service_port)
	if err != nil {
		panic(err)
	}
	go serviceRequest(conn)
	var ret []string
	ret = append(ret, "CONNECT", nodeid, name, arguments[1])
	result := strings.Join(ret, " ")
	mtx.Lock()
	request_chan <- result
	mtx.Unlock()
    go doEvery(5*time.Second, printBalance)
    go doEvery(time.Second, checkConn)
	go miner()
	go blockHandler()
	reader := bufio.NewReader(conn)
	for {
		msg, err := reader.ReadString('\n')
		if err == nil {
			//fmt.Printf("%s\n", msg)
			string_chan <- msg
		} else {
			// fmt.Println(err.Error())
			conn.Close()
			t := time.Now().UnixNano()
			x := float64(t) / float64(1000000000)
			s := fmt.Sprintf("%f", x)
			fmt.Printf("%s\n", s+" - "+"node "+nodeid+" disconnected")
			return
		}
		runtime.Gosched()
	}

}
