package main

import (
	"bufio"
	"fmt"
	"net"
	"net/rpc"
	"os"
	"strconv"
	"strings"
	"time"
)

var Transid int
var ConnGroup []*rpc.Client
var CoordConn *rpc.Client
var processid int
var Intransaction bool

type Resp struct{}
type Req struct {
	Tid       int
	From      string //from which variable
	Amount    int
	Method    string
	Timestamp float64
	Processid int
	Branch    string
}

type RegTrans struct {
	Tid       int
	Processid int
	Hostip    string
	Hostport  string
}

func (R *Resp) GetReply(Reply *string, Dummyvar *string) error {
	fmt.Printf("%s\n", *Reply)
	if *Reply == "NOT FOUND" {
		var reply string
		CoordConn.Call("Resp.AbortTransaction", &Transid, &reply)
		fmt.Println(reply)
		Intransaction = false
	}
	return nil
}

func main() {
	//name, _ := os.Hostname()
	arguments := os.Args
	serverport := ":" + arguments[1]
	hostport := ":" + arguments[2]
	ConnGroup = []*rpc.Client{}
	Branchmap := map[string]int{"A": 0, "B": 1, "C": 2, "D": 3, "E": 4}
	name, _ := os.Hostname()
	host := strings.Split(strings.Split(name, ".")[0], "-")
	id := host[len(host)-1]
	idInt, _ := strconv.Atoi(id)
	processid = idInt - 1
	Intransaction = false
	Rsp := new(Resp)
	rpc.Register(Rsp)
	rpc.HandleHTTP()
	go func() {
		tcpAddr1, err := net.ResolveTCPAddr("tcp", hostport)
		listener1, err := net.ListenTCP("tcp", tcpAddr1)
		// s, err := net.Listen("tcp", port)
		if err != nil {
			fmt.Println("listen fail")
			os.Exit(1)
		}
		// fmt.Println("listen on port: " + port)
		for {
			//fmt.Println("ready for connection")
			conn1, err := listener1.Accept()
			if err != nil {
				fmt.Println("acception fail")
				os.Exit(1)
			}
			go rpc.ServeConn(conn1)
		}
	}()
	//connect with 5 branches
	for i := 1; i <= 5; {
		hostip := "sp20-cs425-g02-0" + strconv.Itoa(i) + ".cs.illinois.edu" + serverport
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
	//connect with coordinator
	for {
		coordip := "sp20-cs425-g02-0" + strconv.Itoa(6) + ".cs.illinois.edu" + serverport
		conn, err := rpc.Dial("tcp", coordip)
		if err != nil {
			continue
		}
		CoordConn = conn
		break
	}
	var dummyin int
	var dummyout int
	CoordConn.Call("Resp.ConnectCheck", &dummyin, &dummyout)
	for {
		//get the input from terminal
		t := time.Now().UnixNano()
		x := float64(t) / float64(1000000000)
		reader := bufio.NewReader(os.Stdin)
		text, _ := reader.ReadString('\n')
		newstring := strings.Split(text, " ") //pay attention to the '\n'
		// fmt.Printf("%s\n", text)
		head := strings.Split(newstring[0], "\n")
		switch head[0] {
		case "BEGIN":
			Intransaction = true
			if len(newstring) != 1 {
				fmt.Println("wrong format!!!!")
				continue
			}
			//call coordinator to get an unique transaction id.
			CoordConn.Call("Resp.AssignTransid", &dummyin, &Transid)
			fmt.Printf("Transaction id : %d\n", Transid)
			//let each server to do initialization
			tempReg := RegTrans{Transid, processid, name, hostport}
			for _, v := range ConnGroup {
				v.Call("Resp.RegisterTrans", &tempReg, &dummyout)
			}
		//send method call to participant with Transid
		case "DEPOSIT":
			if !Intransaction {
				fmt.Println("Type BEGIN to start a transaction")
				continue
			}
			if len(newstring) != 3 {
				fmt.Println("wrong format!!!!")
				continue
			}
			tempstr := strings.Split(newstring[1], ".")
			amt := strings.Split(newstring[2], "\n")
			amount, err := strconv.Atoi(amt[0])
			if err != nil {
				panic(err)
			}
			To := Branchmap[tempstr[0]]
			tempwrite := Req{Transid, tempstr[1], amount, "D", x, processid, tempstr[0]}
			// fmt.Println(tempwrite)
			ConnGroup[To].Call("Resp.RequestTrans", &tempwrite, &dummyout)
			// fmt.Println("after call request")
		case "WITHDRAW":
			if !Intransaction {
				fmt.Println("Type BEGIN to start a transaction")
				continue
			}
			if len(newstring) != 3 {
				fmt.Println("wrong format!!!!")
				continue
			}
			tempstr := strings.Split(newstring[1], ".")
			amt := strings.Split(newstring[2], "\n")
			amount, err := strconv.Atoi(amt[0])
			if err != nil {
				panic(err)
			}
			To := Branchmap[tempstr[0]]
			tempwrite := Req{Transid, tempstr[1], amount, "W", x, processid, tempstr[0]}
			// fmt.Println(tempwrite)
			ConnGroup[To].Call("Resp.RequestTrans", &tempwrite, &dummyout)
			// fmt.Println("after call request")
		case "BALANCE":
			if !Intransaction {
				fmt.Println("Type BEGIN to start a transaction")
				continue
			}
			if len(newstring) != 2 {
				fmt.Println("wrong format!!!!")
				continue
			}
			tempstr := strings.Split(newstring[1], "\n")
			act := strings.Split(tempstr[0], ".")
			To := Branchmap[act[0]]
			tempwrite := Req{Transid, act[1], 0, "B", x, processid, act[0]}
			ConnGroup[To].Call("Resp.RequestTrans", &tempwrite, &dummyout)
		case "COMMIT":
			if !Intransaction {
				fmt.Println("Type BEGIN to start a transaction")
				continue
			}
			if len(newstring) != 1 {
				fmt.Println("wrong format!!!!")
				continue
			}
			var reply string
			CoordConn.Call("Resp.CommitTransaction", &Transid, &reply)
			fmt.Println(reply)
			Intransaction = false
		case "ABORT":
			if !Intransaction {
				fmt.Println("Type BEGIN to start a transaction")
				continue
			}
			if len(newstring) != 1 {
				fmt.Println("wrong format!!!!")
				continue
			}
			var reply string
			CoordConn.Call("Resp.AbortTransaction", &Transid, &reply)
			fmt.Println(reply)
			Intransaction = false
		default:
			fmt.Println("Wrong format!!!!")
		}
	}

}
