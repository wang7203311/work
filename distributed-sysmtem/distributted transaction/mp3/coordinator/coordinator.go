package main

import (
	"fmt"
	"net"
	"net/rpc"
	"os"
	"runtime"
	"strconv"
	"sync"
	"time"
)

type Resp struct{}
type Packet struct {
	From        int     //send from which node
	SequenceNum float64 //the sequence number of the packet
	Message     string  //message of the packet
	Priority    float64 //priority
	Final       bool    //final priority if Final = true
	Replyfrom   int
}

type Req struct {
	Tid       int
	From      string //from which variable
	Amount    int
	Method    string
	Timestamp float64
	Processid int
	Branch    string
}

type ParticipantReq struct {
	Tid     int
	From    int
	Account string
}

var Transid int
var RWmtx sync.RWMutex
var Translistlock sync.RWMutex
var ParticipantMap map[int][]ParticipantReq
var ConnGroup []*rpc.Client
var TransactionList map[int][]Req //key = transactionid, value = list of transaction
//var TidtoVar map[]
//assign an unique id to each transaction
func (R *Resp) AssignTransid(pkt *int, response *int) error {
	RWmtx.Lock()
	Transid++
	*response = Transid
	Translistlock.Lock()
	TransactionList[Transid] = []Req{}
	Translistlock.Unlock()
	RWmtx.Unlock()
	return nil
}

//once receive an abort request, send to all participant to abort transaction
func (R *Resp) AbortReq(pkt *Packet, response *Packet) error {
	return nil
}

// //called by the participant
// func (R *Resp) JoinCoord(pkt *Packet, response *Packet) error {
// 	return nil
// }

//connectivity check
func (R *Resp) ConnectCheck(pkt *int, response *int) error {
	fmt.Println("Coordionator Connect!")
	return nil
}

func (R *Resp) PushtoList(pkt *Req, dommy *int) error {
	Translistlock.Lock()
	TransactionList[pkt.Tid] = append(TransactionList[pkt.Tid], *pkt)
	Translistlock.Unlock()
	return nil
}

func (R *Resp) PoptoList(Tid *int, dommy *int) error {
	Translistlock.Lock()
	TransactionList[*Tid] = TransactionList[*Tid][1:]
	Translistlock.Unlock()
	return nil
}

func (R *Resp) CheckCurrTransaction(pkt *Req, reply *bool) error {
	Translistlock.RLock()
	if TransactionList[pkt.Tid][0] != *pkt {
		*reply = false
	} else {
		*reply = true
	}
	Translistlock.RUnlock()
	return nil
}

func (R *Resp) CommitTransaction(Tid *int, reply *string) error {
	//check if there are any pending commands
	Translistlock.RLock()
	for len(TransactionList[*Tid]) > 0 {
		Translistlock.RUnlock()
		runtime.Gosched()
		time.Sleep(5 * time.Millisecond)
		Translistlock.RLock()
	}
	Translistlock.RUnlock()
	//apply 2pc
	shouldabort := false
	var abortcheck bool
	//phase 1
	for _, v := range ConnGroup {
		v.Call("Resp.PhaseOne", &Tid, &abortcheck)
		if abortcheck == true { //
			shouldabort = true
			break
		}
	}
	//phase 2
	if shouldabort { //ask all participant to abort the transaction and release the lock
		for _, v := range ConnGroup {
			v.Call("Resp.Abortall", &Tid, &abortcheck)
		}
		*reply = "ABORTED"
	} else { //ask all participant to commit the transaction and release the lock
		for _, v := range ConnGroup {
			v.Call("Resp.CommitAll", &Tid, &abortcheck)
			//reply commit ok
		}
		*reply = "COMMIT OK"
	}

	return nil
}

func (R *Resp) AbortTransaction(Tid *int, reply *string) error {
	*reply = "ABORTED"
	var abortcheck bool
	for _, v := range ConnGroup {
		v.Call("Resp.Abortall", Tid, &abortcheck)
	}
	return nil
}

func main() {
	arguments := os.Args
	port := ":" + arguments[1]
	Rsp := new(Resp)
	rpc.Register(Rsp)
	rpc.HandleHTTP()
	Transid = 0
	ConnGroup = []*rpc.Client{}
	ParticipantMap = make(map[int][]ParticipantReq)
	TransactionList = make(map[int][]Req)
	//listen on a port
	// s, _ := net.ResolveTCPAddr("udp", port)
	tcpAddr, err := net.ResolveTCPAddr("tcp", port)
	listener, err := net.ListenTCP("tcp", tcpAddr)
	// s, err := net.Listen("tcp", port)
	if err != nil {
		fmt.Println("listen fail")
		os.Exit(1)
	}
	// fmt.Println("listen on port: " + port)
	go func() {
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
	for i := 1; i <= 5; {
		hostip := "sp20-cs425-g02-0" + strconv.Itoa(i) + ".cs.illinois.edu" + port
		conn, err := rpc.Dial("tcp", hostip)
		if err != nil {
			continue
		} else {
			i++
			var dumin int
			var dumout int
			conn.Call("Resp.ConnectCheck", &dumin, &dumout)
			ConnGroup = append(ConnGroup, conn)
		}
	}
	for {
		runtime.Gosched()
	}

}
