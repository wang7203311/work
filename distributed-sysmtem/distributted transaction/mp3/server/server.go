package main

import (
	"fmt"
	"net"
	"net/rpc"
	"os"
	"runtime"
	"strconv"
	"strings"
	"sync"
	"time"
)

type Lock_t struct {
	Type  string
	Owner []int
}

type ParticipantReq struct {
	Tid     int
	From    int    //from which branch
	Account string //from which account
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

type RegTrans struct {
	Tid       int
	Processid int
	Hostip    string
	Hostport  string
}

var CoordConn *rpc.Client
var processid int

type Resp struct{}

var Lockmtx sync.RWMutex            //lock for the BranchLock
var Tempbranchlock sync.RWMutex     //lock for the TemporaryBranch
var Msbranchlock sync.RWMutex       //lock for the master branhc
var Pidmaplock sync.RWMutex         //lock for the PidtoTrans
var AquriedLock sync.RWMutex        //lock for the AquiredLockList
var checklock sync.RWMutex          //lock for Aquiredlockcheck
var Abortlock sync.RWMutex          //lock for AbortCheck
var waitingaccountlock sync.RWMutex //lock for waitingaccount
var BranchLock map[string][]Lock_t  //key = account, value = list of Lock struct
//temporay copy of the master branch
var TemporaryBranch map[int]map[string]int //key = transaction id. value =  {key = account, value = list of Lock struct and amount}
var MasterBranch map[string]int            //master branch, key = account, val = balance
var AquiredLockList map[int][]string       //key = Tid, val = account
var PidtoTrans map[int]*rpc.Client
var AbortCheck map[int]bool                  //key = Tid, val = whether this transaction is aborted
var Aquiredlockcheck map[int]map[string]bool //key = Tid, value = {key:account, val = whether already in the Aquiredlocklist}
var waitingaccount map[int]string            //val = Tid, val = the name of the waiting account
func CanGetLock(locklist Lock_t, pkt Req, Type string) bool {
	ownerlist := locklist.Owner
	if len(ownerlist) == 0 { // no lock on this variable
		BranchLock[pkt.From][0].Type = Type
		BranchLock[pkt.From][0].Owner = append(BranchLock[pkt.From][0].Owner, pkt.Tid)
		fmt.Println("empty owner")
		return true
	}
	//for write lock
	if Type == "W" && len(ownerlist) > 1 { //cannot upgrade the lock because it is a shared lock
		return false
	} else if Type == "W" && len(ownerlist) == 1 && ownerlist[0] == pkt.Tid { //can get the lock because I am the only owner
		BranchLock[pkt.From][0].Type = "W" //upgrade or maintain the lock
		return true
	}
	//for shared lock (read lock)
	if Type == "R" {
		if locklist.Type == "R" { // the variable is locked by shared lock
			for _, v := range ownerlist {
				if v == pkt.Tid { //the transaction id is in the owner list
					return true
				}
			}
			//add me to the owner list
			BranchLock[pkt.From][0].Owner = append(BranchLock[pkt.From][0].Owner, pkt.Tid)
			return true
		} else { // the variable is locked by write lock
			return pkt.Tid == ownerlist[0]
		}
	}
	return false
}

//write operation
func Deposit(pkt Req) {
	//check the current command
	var IscurrentTrans bool
	for {
		CoordConn.Call("Resp.CheckCurrTransaction", &pkt, &IscurrentTrans)
		if IscurrentTrans {
			break
		}
		Abortlock.Lock()
		if AbortCheck[pkt.Tid] == true { //this transaction is aborted
			Abortlock.Unlock()
			return
		}
		Abortlock.Unlock()
		runtime.Gosched()
	}
	fmt.Printf("current transaction: ")
	fmt.Println(pkt)
	//join
	//acquire the lock(check if there is such account)
	tempowner := []int{pkt.Tid}
	templocklist := []Lock_t{{"W", tempowner}}
	Lockmtx.Lock()
	templ, ok := BranchLock[pkt.From] //check lock condition
	if !ok {                          //new account
		BranchLock[pkt.From] = templocklist //get the lock on the shared resource

		Lockmtx.Unlock()
		Tempbranchlock.Lock()
		TemporaryBranch[pkt.Tid][pkt.From] = pkt.Amount //update this result in the temporary branch
		Tempbranchlock.Unlock()
	} else if len(templ) == 0 { //can get the lock
		templock := Lock_t{"W", []int{pkt.Tid}}
		BranchLock[pkt.From] = []Lock_t{templock}
		Lockmtx.Unlock()

		Tempbranchlock.Lock()
		TemporaryBranch[pkt.Tid][pkt.From] += pkt.Amount
		Tempbranchlock.Unlock()
	} else if CanGetLock(templ[0], pkt, "W") { //get the lock and update transaction
		Lockmtx.Unlock()
		Tempbranchlock.Lock()
		TemporaryBranch[pkt.Tid][pkt.From] += pkt.Amount
		Tempbranchlock.Unlock()
	} else { //cannot get the lock, push it to the end
		templockl := Lock_t{"W", tempowner}
		BranchLock[pkt.From] = append(BranchLock[pkt.From], templockl)
		Lockmtx.Unlock()
		//wait until get the lock
		waitingaccountlock.Lock()
		waitingaccount[pkt.Tid] = pkt.From + "+W"
		waitingaccountlock.Unlock()
		fmt.Printf("Tid %d wait for lock\n", pkt.Tid)
		Lockmtx.RLock()
		for {
			if BranchLock[pkt.From][0].Owner[0] == pkt.Tid && BranchLock[pkt.From][0].Type == "W" {
				break
			}
			Lockmtx.RUnlock()
			runtime.Gosched()
			Abortlock.Lock()
			if AbortCheck[pkt.Tid] == true { //this transaction is aborted
				Abortlock.Unlock()
				return
			}
			Abortlock.Unlock()
			time.Sleep(5 * time.Millisecond)
			Lockmtx.RLock()
		}
		Lockmtx.RUnlock()
		//update
		Tempbranchlock.Lock()
		TemporaryBranch[pkt.Tid][pkt.From] += pkt.Amount
		Tempbranchlock.Unlock()

	}
	fmt.Printf("Tid %d own the lock\n", pkt.Tid)
	waitingaccountlock.Lock()
	waitingaccount[pkt.Tid] = "NO WAITING ACCOUNT"
	waitingaccountlock.Unlock()
	//push to the acquried lock list
	checklock.Lock()
	_, okcheck := Aquiredlockcheck[pkt.Tid][pkt.From]
	if !okcheck { //do not aquire this lock before
		Aquiredlockcheck[pkt.Tid][pkt.From] = true
		AquriedLock.Lock()
		AquiredLockList[pkt.Tid] = append(AquiredLockList[pkt.Tid], pkt.From)
		AquriedLock.Unlock()
	}
	checklock.Unlock()
	//response ok
	reply := "OK"
	var dumstr string
	PidtoTrans[pkt.Processid].Call("Resp.GetReply", &reply, &dumstr)
	//popout from the transactionlist
	var dummyint int
	CoordConn.Call("Resp.PoptoList", &(pkt.Tid), &dummyint)
}

//write operation
func Widthdraw(pkt Req) {
	//check if we need to execute current transaction
	var IscurrentTrans bool
	for {
		CoordConn.Call("Resp.CheckCurrTransaction", &pkt, &IscurrentTrans)
		if IscurrentTrans {
			break
		}
		Abortlock.Lock()
		if AbortCheck[pkt.Tid] == true { //this transaction is aborted
			Abortlock.Unlock()
			return
		}
		Abortlock.Unlock()
		runtime.Gosched()
	}
	fmt.Printf("current transaction: ")
	fmt.Println(pkt)

	//validation: check the integrity of the account
	//if there is no such accont in tempbranch and master branch
	Msbranchlock.RLock()
	_, ok := MasterBranch[pkt.From]
	Msbranchlock.RUnlock()
	Tempbranchlock.RLock()
	_, ok2 := TemporaryBranch[pkt.Tid][pkt.From]
	Tempbranchlock.RUnlock()
	var reply string
	var dumstr string
	if !ok && !ok2 {
		reply = "NOT FOUND"
		//ABORT
		PidtoTrans[pkt.Processid].Call("Resp.GetReply", &reply, &dumstr)
		return
	}
	//acquire the lock(check if there is such account, if not->abort)
	tempowner := []int{pkt.Tid}
	//templocklist := []Lock_t{{"W", tempowner}}
	Lockmtx.Lock()
	templ, ok := BranchLock[pkt.From] //check lock condition
	if !ok {                          //new account
		reply = "NOT FOUND"
		//ABORT
		Lockmtx.Unlock()
		PidtoTrans[pkt.Processid].Call("Resp.GetReply", &reply, &dumstr)
		return
	} else if len(templ) == 0 { //can get the lock
		templock := Lock_t{"W", []int{pkt.Tid}}
		BranchLock[pkt.From] = []Lock_t{templock}
		Lockmtx.Unlock()
		Tempbranchlock.Lock()
		TemporaryBranch[pkt.Tid][pkt.From] -= pkt.Amount
		Tempbranchlock.Unlock()
	} else if CanGetLock(templ[0], pkt, "W") { //get the lock and update transaction
		Lockmtx.Unlock()
		Tempbranchlock.Lock()
		TemporaryBranch[pkt.Tid][pkt.From] -= pkt.Amount
		Tempbranchlock.Unlock()
	} else { //cannot get the lock, push it to the end
		templockl := Lock_t{"W", tempowner}
		BranchLock[pkt.From] = append(BranchLock[pkt.From], templockl)
		Lockmtx.Unlock()
		//wait until get the lock
		waitingaccountlock.Lock()
		waitingaccount[pkt.Tid] = pkt.From + "+W"
		waitingaccountlock.Unlock()
		fmt.Printf("Tid %d wait for lock\n", pkt.Tid)
		Lockmtx.RLock()
		for {
			if BranchLock[pkt.From][0].Owner[0] == pkt.Tid && BranchLock[pkt.From][0].Type == "W" {
				break
			}
			Lockmtx.RUnlock()
			runtime.Gosched()
			Abortlock.Lock()
			if AbortCheck[pkt.Tid] == true { //this transaction is aborted
				Abortlock.Unlock()
				return
			}
			Abortlock.Unlock()
			time.Sleep(5 * time.Millisecond)
			Lockmtx.RLock()
		}
		Lockmtx.RUnlock()
		//update
		Tempbranchlock.Lock()
		TemporaryBranch[pkt.Tid][pkt.From] -= pkt.Amount
		Tempbranchlock.Unlock()

	}
	fmt.Printf("Tid %d own the lock\n", pkt.Tid)
	waitingaccountlock.Lock()
	waitingaccount[pkt.Tid] = "NO WAITING ACCOUNT"
	waitingaccountlock.Unlock()
	//push to the acquried lock list
	checklock.Lock()
	_, okcheck := Aquiredlockcheck[pkt.Tid][pkt.From]
	if !okcheck { //do not aquire this lock before
		Aquiredlockcheck[pkt.Tid][pkt.From] = true
		AquriedLock.Lock()
		AquiredLockList[pkt.Tid] = append(AquiredLockList[pkt.Tid], pkt.From)
		AquriedLock.Unlock()
	}
	checklock.Unlock()
	//response ok
	reply = "OK"
	PidtoTrans[pkt.Processid].Call("Resp.GetReply", &reply, &dumstr)
	//popout from the transactionlist
	var dummyint int
	CoordConn.Call("Resp.PoptoList", &(pkt.Tid), &dummyint)

}

func Intheownerlist(ownerlist []int, Tid int) bool {
	for _, v := range ownerlist {
		if v == Tid {
			return true
		}
	}
	return false
}

//read operation
func BalanceCheck(pkt Req) {
	//check if we need to execute current transaction
	var IscurrentTrans bool
	for {
		CoordConn.Call("Resp.CheckCurrTransaction", &pkt, &IscurrentTrans)
		if IscurrentTrans {
			break
		}
		Abortlock.Lock()
		if AbortCheck[pkt.Tid] == true { //this transaction is aborted
			Abortlock.Unlock()
			return
		}
		Abortlock.Unlock()
		runtime.Gosched()
	}
	fmt.Printf("current transaction: ")
	fmt.Println(pkt)

	//validation: check the integrity of the account
	//if there is no such accont in tempbranch and master branch
	Msbranchlock.RLock()
	balanceinmaster, ok := MasterBranch[pkt.From]
	Msbranchlock.RUnlock()
	Tempbranchlock.RLock()
	balanceintemp, ok2 := TemporaryBranch[pkt.Tid][pkt.From]
	Tempbranchlock.RUnlock()
	var reply string
	var dumstr string
	if !ok && !ok2 {
		reply = "NOT FOUND"
		//ABORT
		PidtoTrans[pkt.Processid].Call("Resp.GetReply", &reply, &dumstr)
		return
	}
	//acquire the lock(check if there is such account, if not->abort)
	tempowner := []int{pkt.Tid}
	templocklist := Lock_t{"R", tempowner}
	Lockmtx.Lock()
	templ, ok := BranchLock[pkt.From]
	if !ok {
		reply = "NOT FOUND"
		//ABORT
		Lockmtx.Unlock()
		PidtoTrans[pkt.Processid].Call("Resp.GetReply", &reply, &dumstr)
		return
	} else if len(templ) == 0 { //can get the lock
		fmt.Printf("Tid %d : no one get the lock\n", pkt.Tid)
		templock := Lock_t{"R", []int{pkt.Tid}}
		BranchLock[pkt.From] = []Lock_t{templock}
		Lockmtx.Unlock()
	} else if CanGetLock(templ[0], pkt, "R") {
		fmt.Printf("Tid %d : already have the lock\n", pkt.Tid)
		fmt.Println(templ[0])
		//read the value
		Lockmtx.Unlock()
	} else { //cannot get the lock, push it to the end
		//loop through the locklist, append into a readlock if there is, othewise, append to the end
		found := false
		for i, v := range BranchLock[pkt.From] {
			if v.Type == "R" {
				BranchLock[pkt.From][i].Owner = append(BranchLock[pkt.From][i].Owner, pkt.Tid)
				found = true
				break
			}
		}
		if !found {
			BranchLock[pkt.From] = append(BranchLock[pkt.From], templocklist)
		}
		Lockmtx.Unlock()
		fmt.Printf("Tid %d wait the lock\n", pkt.Tid)
		//wait until get the lock
		waitingaccountlock.Lock()
		waitingaccount[pkt.Tid] = pkt.From + "+R"
		waitingaccountlock.Unlock()
		Lockmtx.RLock()
		for {
			if Intheownerlist(BranchLock[pkt.From][0].Owner, pkt.Tid) {
				break
			}
			Lockmtx.RUnlock()
			runtime.Gosched()
			Abortlock.Lock()
			if AbortCheck[pkt.Tid] == true { //this transaction is aborted
				Abortlock.Unlock()
				return
			}
			Abortlock.Unlock()
			time.Sleep(5 * time.Millisecond)
			Lockmtx.RLock()
		}
		Lockmtx.RUnlock()
		fmt.Printf("Tid %d own the lock\n", pkt.Tid)
	}
	//push to the acquried lock list
	waitingaccountlock.Lock()
	waitingaccount[pkt.Tid] = "NO WAITING ACCOUNT"
	waitingaccountlock.Unlock()
	checklock.Lock()
	_, okcheck := Aquiredlockcheck[pkt.Tid][pkt.From]
	if !okcheck { //do not aquire this lock before
		Aquiredlockcheck[pkt.Tid][pkt.From] = true
		AquriedLock.Lock()
		AquiredLockList[pkt.Tid] = append(AquiredLockList[pkt.Tid], pkt.From)
		AquriedLock.Unlock()
	}
	checklock.Unlock()

	Msbranchlock.RLock()
	balanceinmaster, ok = MasterBranch[pkt.From]
	Msbranchlock.RUnlock()
	Tempbranchlock.RLock()
	balanceintemp, ok2 = TemporaryBranch[pkt.Tid][pkt.From]
	Tempbranchlock.RUnlock()
	//response
	reply = pkt.Branch + "." + pkt.From + " = "
	totalamount := 0
	if ok && ok2 { //both branch have value
		totalamount += balanceinmaster + balanceintemp
	} else if ok && !ok2 {
		totalamount += balanceinmaster
	} else if !ok && ok2 {
		totalamount += balanceintemp
	}
	numberinstr := strconv.Itoa(totalamount)
	reply += numberinstr
	PidtoTrans[pkt.Processid].Call("Resp.GetReply", &reply, &dumstr)
	var dummyint int
	CoordConn.Call("Resp.PoptoList", &(pkt.Tid), &dummyint)
}

//connectivity check
func (R *Resp) RequestTrans(pkt *Req, response *int) error {
	var dummy int
	CoordConn.Call("Resp.PushtoList", pkt, &dummy)
	//fmt.Println(*pkt)
	switch pkt.Method {
	case "D":
		go Deposit(*pkt)
	case "B":
		go BalanceCheck(*pkt)
	case "W":
		go Widthdraw(*pkt)
	}
	return nil
}

//connectivity check
func (R *Resp) ConnectCheck(pkt *int, response *int) error {
	fmt.Println("Server Connect!")
	return nil
}

//registe the transaction
func (R *Resp) RegisterTrans(pkt *RegTrans, response *int) error {
	Tempbranchlock.Lock()
	TemporaryBranch[pkt.Tid] = make(map[string]int)
	Tempbranchlock.Unlock()
	checklock.Lock()
	Aquiredlockcheck[pkt.Tid] = make(map[string]bool)
	checklock.Unlock()

	Abortlock.Lock()
	AbortCheck[pkt.Tid] = false
	Abortlock.Unlock()

	waitingaccountlock.Lock()
	waitingaccount[pkt.Tid] = "NO WAITING ACCOUNT"
	waitingaccountlock.Unlock()

	//check
	Pidmaplock.Lock()
	if _, ok := PidtoTrans[pkt.Processid]; !ok { //do not establish connection before
		for {
			conn, err := rpc.Dial("tcp", pkt.Hostip+pkt.Hostport)
			if err != nil {
				continue
			}
			PidtoTrans[pkt.Processid] = conn
			break
		}
	}
	Pidmaplock.Unlock()
	return nil
}

//do the phaseone check
func (R *Resp) PhaseOne(Tid *int, shouldabout *bool) error {
	//check if we will have negative transaction
	Tempbranchlock.RLock()
	Msbranchlock.RLock()
	*shouldabout = false
	for key, val := range TemporaryBranch[*Tid] {
		balance, ok := MasterBranch[key]
		if !ok { // the master branch do not such account
			if val < 0 {
				*shouldabout = true
				Msbranchlock.RUnlock()
				Tempbranchlock.RUnlock()
				return nil
			}
		} else { //check if the combined result is negative
			if balance+val < 0 {
				*shouldabout = true
				Msbranchlock.RUnlock()
				Tempbranchlock.RUnlock()
				return nil
			}
		}
	}
	Msbranchlock.RUnlock()
	Tempbranchlock.RUnlock()
	return nil
}

func ReleaseLock(Tid int) {
	//release the lock
	Lockmtx.Lock()
	for _, val := range AquiredLockList[Tid] { //val = account which owns the lock now
		tempcheck := BranchLock[val][0]
		if tempcheck.Type == "R" { //this is a read lock
			if len(tempcheck.Owner) == 1 {
				BranchLock[val] = BranchLock[val][1:] //pop
			} else { //loop the owner list and remove it
				for i, v := range tempcheck.Owner {
					if v == Tid {
						fmt.Println("find it!")
						BranchLock[val][0].Owner[i] = BranchLock[val][0].Owner[len(tempcheck.Owner)-1]
						BranchLock[val][0].Owner = BranchLock[val][0].Owner[:len(tempcheck.Owner)-1]
						break
					}
				}
				//check if we can change the some read lock to write lock
				if len(BranchLock[val][0].Owner) == 1 {
					remainingtid := BranchLock[val][0].Owner[0]
					for i, v := range BranchLock[val] {
						if v.Type == "W" && v.Owner[0] == remainingtid {
							BranchLock[val][0].Type = "W"
							BranchLock[val] = append(BranchLock[val][:i], BranchLock[val][i+1:]...)
							break
						}
					}

				}
			}
		} else { //this is a write lock
			BranchLock[val] = BranchLock[val][1:] //pop
		}
	}
	Lockmtx.Unlock()
}

//shouldabort is a dummy variable in this rpc call
func (R *Resp) CommitAll(Tid *int, shouldabout *bool) error {
	Tempbranchlock.RLock()
	Msbranchlock.Lock()
	*shouldabout = false
	for key, val := range TemporaryBranch[*Tid] { //key  = account name
		balance, ok := MasterBranch[key]
		//commit to the master branch
		if !ok { // the master branch do not such account
			MasterBranch[key] = val
		} else { //check if the combined result is negative
			MasterBranch[key] = balance + val
		}
	}
	Msbranchlock.Unlock()
	Tempbranchlock.RUnlock()

	//release all aquired lock
	ReleaseLock(*Tid)
	return nil
}

//shouldabort is a dummy variable in this rpc call
func (R *Resp) Abortall(Tid *int, shouldabout *bool) error {
	fmt.Println("Abort all !!!")
	//TODO:
	//kill all waiting commands
	Abortlock.Lock()
	AbortCheck[*Tid] = true
	Abortlock.Unlock()

	//release all aquired lock
	ReleaseLock(*Tid)

	waitingaccountlock.Lock()
	if waitingaccount[*Tid] != "NO WAITING ACCOUNT" {
		Lockmtx.Lock()
		newstring := strings.Split(waitingaccount[*Tid], "+")
		if newstring[1] == "W" {
			fmt.Println("waiting lock is W")
			for i, v := range BranchLock[newstring[0]] {
				if v.Owner[0] == *Tid { //find
					BranchLock[newstring[0]] = append(BranchLock[newstring[0]][:i], BranchLock[newstring[0]][i+1:]...)
					break
				}
			}
		} else {
			fmt.Println("waiting lock is R")
			idx := 0
			for i, v := range BranchLock[newstring[0]] {
				if v.Type == "R" {
					idx = i
				}
			}
			for i, b := range BranchLock[newstring[0]][idx].Owner {
				if b == *Tid {
					BranchLock[newstring[0]][idx].Owner = append(BranchLock[newstring[0]][idx].Owner[:i], BranchLock[newstring[0]][idx].Owner[i+1:]...)
				}
			}

		}
		Lockmtx.Unlock()
	}
	waitingaccountlock.Unlock()
	return nil
}

func main() {
	arguments := os.Args
	port := ":" + arguments[1]
	name, _ := os.Hostname()
	host := strings.Split(strings.Split(name, ".")[0], "-")
	id := host[len(host)-1]
	idInt, _ := strconv.Atoi(id)
	processid = idInt - 1
	fmt.Println(processid)
	Rsp := new(Resp)
	rpc.Register(Rsp)
	rpc.HandleHTTP()
	MasterBranch = make(map[string]int)
	TemporaryBranch = make(map[int]map[string]int)
	BranchLock = make(map[string][]Lock_t)
	PidtoTrans = make(map[int]*rpc.Client)
	AquiredLockList = make(map[int][]string)
	AbortCheck = make(map[int]bool)
	Aquiredlockcheck = make(map[int]map[string]bool)
	waitingaccount = make(map[int]string)
	//connect with coordinator
	for {
		coordip := "sp20-cs425-g02-0" + strconv.Itoa(6) + ".cs.illinois.edu" + port
		conn, err := rpc.Dial("tcp", coordip)
		if err != nil {
			continue
		}
		CoordConn = conn
		break
	}
	var dumin int
	var dumout int
	CoordConn.Call("Resp.ConnectCheck", &dumin, &dumout)
	//listen on port
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

}
