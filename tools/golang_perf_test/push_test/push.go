package main

import (
  "flag"
  "bufio"
  "bytes"
  "encoding/json"
  "fmt"
  "io"
  "io/ioutil"
  "net"
  "net/http"
  "os"
  "runtime"
  "strconv"
  "strings"
  "time"
)

const (
  RecvBufLen           = 4096
  MaxRecvSubMsgNum     = 10
  HeartBeatSendTimeSec = 100
  MsgTypeLogin         = "login"
  MsgTypeMsg           = "msg"
  MsgTypeHeartBeat     = "noop"
  MsgTypeAck           = "ack"
  MsgTypeClosed        = "closed"
  LoginRespVerify      = `HTTP/1.1 200 OK`
  statsFileName        = `stats`
  StatsSingleUserInfo  = `single`
  StatsTotalUserInfo   = `total`
  StatsTotalStatsInfo  = `stats`
  StatsOrderExit       = `exit`
  StatsClosed          = `closed`
  TimeOutMaxNum        = 1
  ConnTimeOutNum       = 100 * time.Second
  ConnClosedMsg        = "1"
  MaxSendMsgErrCount   = 5
)
var host1 string = `localhost:9003`
var host2 string = `localhost:9001`
var session_host1 string = `localhost:9002`
var session_host2 string = `localhost:9000`
var g_log_switch bool = false
/*
type Ack struct {
  Type string `json:"type"`
  From string `json:"from"`
  Seq  int64  `json:"seq"`
}
*/
type BodyResult struct {
  Result []string `json:"result"`
}
type Msg struct {
  Type int    `json:"y"`
  To   string `json:"t"`
  Body string `json:"b"`
  Seq  int64  `json:"s"`
  From string `json:"f"`
}

/*
type HeartBeatMsg struct {
  Type string `json:"type"`
}
*/
type OrderMsg struct {
  Type     string `json:"type"`
  UserName string `json:"userName"`
  To       string `json:"to"`
  Body     string `json:"body"`
}

type MsgSend struct {
  To   string `json:"to"`
  Body string `json:"body"`
}

type Shard struct {
  MsgUrl     string
  ChannelUrl string
  SubUrl     string
  UnsubUrl   string
}

type ClientInfo struct {
  UserName                string
  Pass                    string
  Status                  string
  TimeUnix                int64
  MsgId                   int64
  ErrMsgId                int64
  SuccMsgId               int64
  WarnMsgId               int64
  LoginStatusId           int32
  UnConnectId             int32
  SendHeartBeatRecvSuccId int32
  AckSuccId               int32
  Seq                     int64
  LoginErrId              int32
  JsonErr                 int32
  SendCh                  chan string
  RecvCh                  chan string
  ExitCh                  chan string
  Conn                    net.Conn
  BreakLoop               bool
  Case                    string
  Shard                   *Shard
}

var clientInfoMap map[string]*ClientInfo

func main() {
  defer func() { //必须要先声明defer，否则不能捕获到panic异常
    if err := recover(); err != nil {
      fmt.Println("err:%v", err) //这里的err其实就是panic传入的内容
    }
  }()
  numCpu := runtime.NumCPU()
  runtime.GOMAXPROCS(numCpu)
  checkServerAddr := flag.String("r", "", "to do check server addr,eg:127.0.0.1")
  verbos := flag.String("v", "0", "1:open the log;0:close the log")
  flag.Parse()
  if *verbos == "1" {
    g_log_switch = true
  }
  hostArr := strings.Split(*checkServerAddr, ",")  
  if len(hostArr) == 1 {
      if !strings.Contains(hostArr[0], ".") {
        fmt.Println("-r must fill![192.168.1.60,192.168.1.60]or[192.168.1.60]")
        return
      }
      host2 = hostArr[0] + ":9001"
      host1 = hostArr[0] + ":9001"
      session_host1 = hostArr[0] + ":9000"
      session_host2 = hostArr[0] + ":9000"
  }
  if len(hostArr) == 2 {
      host2 = hostArr[0] + ":9001"
      host1 = hostArr[1] + ":9003"
      session_host1 = hostArr[1] + ":9002"
      session_host2 = hostArr[0] + ":9000"
  }

  if len(hostArr) > 2 || len(hostArr) < 1 {
      fmt.Println("-r must fill correct!")
      return
  }
  userName1 := "test_1"
  userName2 := "test_2"
  fmt.Println("begin check server")
  offlineUrl1 := `http://` + host1 + `/msg?uid=` + userName1
  offlineUrl2 := `http://` + host2 + `/msg?uid=` + userName2
  msgUrl1 := `http://` + host1 + `/pub?to=` + userName1 + `&from=op`
  msgUrl2 := `http://` + host2 + `/pub?to=` + userName2 + `&from=op`
  channelUrl1 := `http://` + host1 + `/pub?channel=channel1&from=op`
  otherChannelUrl1 := `http://` + host1 + `/pub?channel=channel1&from=op`
  channelUrl2 := `http://` + host2 + `/pub?channel=channel1&from=op`
  otherChannelUrl2 := `http://` + host2 + `/pub?channel=channel2&from=op`
  subUrl1 := `http://` + host1 + `/sub?uid=` + userName1 + `&cid=channel1`
  otherSubUrl1 := `http://` + host1 + `/sub?uid=` + userName1 + `&cid=channel1`
  subUrl2 := `http://` + host2 + `/sub?uid=` + userName2 + `&cid=channel1`
  otherSubUrl2 := `http://` + host2 + `/sub?uid=` + userName2 + `&cid=channel2`
  unsubUrl1 := `http://` + host1 + `/unsub?uid=` + userName1 + `&cid=channel1`
  otherUnsub1 := `http://` + host1 + `/unsub?uid=` + userName1 + `&cid=channel1`
  unsubUrl2 := `http://` + host2 + `/unsub?uid=` + userName2 + `&cid=channel1`
  otherUnsub2 := `http://` + host2 + `/unsub?uid=` + userName2 + `&cid=channel2`

  client1 := &ClientInfo{}
  client1.Shard = &Shard{}
  client1.UserName = userName1
  client1.Pass = client1.UserName
  client1.Shard.MsgUrl = `http://` + host2 + `/pub?to=` + userName1 + `&from=op`
  client1.Shard.ChannelUrl = `http://` + host2 + `/pub?channel=channel1&from=op`
  client1.Shard.SubUrl = `http://` + host2 + `/sub?uid=` + userName1 + `&cid=channel1`
  client1.Shard.UnsubUrl = `http://` + host2 + `/unsub?uid=` + userName1 + `&cid=channel1`
  clientInfoMap = make(map[string]*ClientInfo)
  clientInfoMap[client1.UserName] = client1
  client2 := &ClientInfo{}
  client2.Shard = &Shard{}
  client2.UserName = userName2
  client2.Pass = client2.UserName
  client2.Shard.MsgUrl = `http://` + host1 + `/pub?to=` + userName2 + `&from=op`
  client2.Shard.ChannelUrl = `http://` + host1 + `/pub?channel=channel2&from=op`
  client2.Shard.SubUrl = `http://` + host1 + `/sub?uid=` + userName2 + `&cid=channel2`
  client2.Shard.UnsubUrl = `http://` + host1 + `/unsub?uid=` + userName2 + `&cid=channel2`
  clientInfoMap[client2.UserName] = client2

  go handleClient(1, session_host1, userName1, offlineUrl1, msgUrl1, channelUrl1, subUrl1, unsubUrl1, client1, otherChannelUrl1, otherSubUrl1, otherUnsub1)
  go handleClient(2, session_host2, userName2, offlineUrl2, msgUrl2, channelUrl2, subUrl2, unsubUrl2, client2, otherChannelUrl2, otherSubUrl2, otherUnsub2)

  for {
    if client1.Status == MsgTypeClosed && client2.Status == MsgTypeClosed {
      break
    }
    time.Sleep(2 * time.Second)
  }
}

func statsClientInfo(w http.ResponseWriter, r *http.Request) {
  body, err := ioutil.ReadAll(r.Body)
  if err != nil {
    fmt.Println("accept order error, err:%v", err)
    w.Write([]byte("order is error!"))
    return
  }
  msg := &OrderMsg{}
  err = json.Unmarshal(body, msg)
  if err != nil {
    fmt.Println("order unmarshal error,body:%s, err:%v", string(body), err)
    w.Write([]byte("order unmarshal is error!"))
    return
  }
  switch msg.Type {
  case StatsSingleUserInfo:
    var strStats bytes.Buffer
    value := clientInfoMap[msg.UserName]
    if value == nil {
      w.Write([]byte("no user:" + msg.UserName))
      return
    }

    fmt.Fprintf(&strStats, "userName:%s\npass:%s\nerr:%d\nwarn:%d\nsucc:%d\n", value.UserName, value.Pass, value.ErrMsgId, value.WarnMsgId, value.SuccMsgId)
    fmt.Fprintf(&strStats, "loginNum:%d\nsendHeartBeatSucc:%d\nUnconnect:%d\n", value.LoginStatusId, value.SendHeartBeatRecvSuccId, value.UnConnectId)
    fmt.Fprintf(&strStats, "ackSucc:%d\nstatus:%s\njsonErr:%d\n", value.AckSuccId, value.Status, value.JsonErr)
    w.Write(strStats.Bytes())
    return
  case StatsTotalUserInfo:
    fileName := statsFileName + "_" + strconv.Itoa(int(time.Now().Unix())) + ".dat"
    go writeStatsToFile(fileName)
    w.Write([]byte("writing status to file:%s, please see it wait a moment!" + fileName))
    return
  case StatsTotalStatsInfo:
    var strStats bytes.Buffer
    var userNums, loginNum, sendSuccHBs, unConnect, ackSucc, statusClosed, logInErr, jsonErr int32
    var errMsg, warn, succ int64
    for _, value := range clientInfoMap {
      userNums += 1
      if value.Status == StatsClosed {
        statusClosed += 1
        continue
      }

      errMsg += value.ErrMsgId
      warn += value.WarnMsgId
      succ += value.SuccMsgId
      loginNum += value.LoginStatusId
      sendSuccHBs += value.SendHeartBeatRecvSuccId
      unConnect += value.UnConnectId
      ackSucc += value.AckSuccId
      logInErr += value.LoginErrId
      jsonErr += value.JsonErr
    }
    fmt.Fprintf(&strStats, "userNums:%d\nerr:%d\nwarn:%d\nsucc:%d\nclosed:%d\nloginErr:%d\njsonErr:%d\n", userNums, errMsg, warn, succ, statusClosed, logInErr, jsonErr)
    fmt.Fprintf(&strStats, "loginNum:%d\nsendHeartBeatSucc:%d\nUnconnect:%d\n", loginNum, sendSuccHBs, unConnect)
    fmt.Fprintf(&strStats, "ackSucc:%d\n", ackSucc)
    w.Write(strStats.Bytes())
    return
  }
}

func writeStatsToFile(fileName string) {
  f, err := os.OpenFile(fileName, os.O_CREATE|os.O_RDWR, 0666)
  if err != nil {
    panic(err)
    return
  }
  defer f.Close()
  var strStats bytes.Buffer
  for _, value := range clientInfoMap {
    fmt.Fprintf(&strStats, "userName:%s\npass:%s\nerr:%d\nwarn:%d\nsucc:%d\n", value.UserName, value.Pass, value.ErrMsgId, value.WarnMsgId, value.SuccMsgId)
    fmt.Fprintf(&strStats, "loginNum:%d\nsendHeartBeatSucc:%d\nUnconnect:%d\n", value.LoginStatusId, value.SendHeartBeatRecvSuccId, value.UnConnectId)
    fmt.Fprintf(&strStats, "ackSucc:%d\nstatus:%s\njsonErr:%d\n", value.AckSuccId, value.Status, value.JsonErr)
  }
  w := bufio.NewWriter(f) //创建新的 Writer 对象
  c, err := w.WriteString(strStats.String())
  if err != nil || c <= 0 {
    fmt.Println("write to file is error, err:%s" + err.Error())
    return
  }
  w.Flush()
}

func connectServer(checkServerAddr string) net.Conn {
  var remoteaddr net.TCPAddr
  var conn net.Conn
  var err error
  conn, err = net.Dial("tcp", checkServerAddr)

  if err != nil {
    fmt.Println("%v connect server error,err:%v", remoteaddr, err)
    return nil
  }
  return conn
}

func getUserOfflineMsg(url string) bool {
  return verifyUserOfflineMsg(url)
}

func verifyUserOfflineMsg(url string) bool {
  response, err := http.Get(url)
  if err != nil {
    fmt.Println("get offline msg is error,err:%v", err)
    return false
  }
  defer response.Body.Close()
  body, err := ioutil.ReadAll(response.Body)
  if err != nil {
    fmt.Println("get offline body is error,err:%v", err)
    return false
  }
  bodyResult := &BodyResult{}
  err = json.Unmarshal(body, bodyResult)
  if err != nil {
    fmt.Println("get offline body unmarshal is error,err:%v", err)
    return false
  }
  if len(bodyResult.Result) == 0 {
    return false
  }

  msg := &Msg{}
  err = json.Unmarshal([]byte(bodyResult.Result[0]), msg)
  if err != nil {
    fmt.Println("get offline body unmarshal msg is error,err:%v", err)
    return false
  }

  if msg.Body != "" {
    return true
  }

  return false
}

func optChannelMsg(url string) bool {
  response, err := http.Get(url)
  if err != nil {
    fmt.Println("get offline msg is error,err:%v", err)
    return false
  }
  defer response.Body.Close()
  _, err = ioutil.ReadAll(response.Body)
  if err != nil {
    fmt.Println("get offline body is error,err:%v", err)
    return false
  }
  return true
}

func sendUserMsg(msgUrl, p string) bool {

  msg := []byte(p + p + "this is a test message")
  body := bytes.NewBuffer(msg)
  res, err := http.Post(msgUrl, "application/json;charset=utf-8", body)
  if err != nil {
    fmt.Println("body:%s,err:%v,msgUrl:%s", body, err, msgUrl)
    return false
  }
  result, err := ioutil.ReadAll(res.Body)
  if err != nil {
    fmt.Println("read resp err:%v", err)
    return false
  }
  //fmt.Println("msg:%s,result:%s", msgUrl, string(result))

  if !strings.Contains(string(result), "ok") {
    fmt.Println("ok resp err,response:%s", string(result))
    return false
  }
  return true
}

func testCase1(checkServerAddr string, userName string, client *ClientInfo) (net.Conn, bool) {
  conn := connectServer(checkServerAddr)
  if conn == nil {
    fmt.Println("connect server is error, addr:%v", checkServerAddr)
    return nil, false
  }
  statusFlag := sendLoginMsg(conn, userName, client)
  if !statusFlag {
    fmt.Println("case 1:login error")
    exit(conn, client)
    return nil, false
  }
  return conn, true
}

func testCase2(conn net.Conn, client *ClientInfo, msgUrl string) bool {
  flag := sendMsg(msgUrl, client, conn)
  if !flag {
    fmt.Println("case 5 fail")
    return false
  }
  return true
}

func testCase5(conn net.Conn, client *ClientInfo, msgUrl string) bool {
  for i := 0; i < 5; i = i + 1 {
    go recvMsgAndSendAck(conn, client)
    flag := sendUserMsg(msgUrl, "case5")
    if !flag {
      fmt.Println("case 5 fail")
      return false
    }
    time.Sleep(5 * time.Second)
  }
  return true
}

func testCase7(msgUrl, offlineUrl string) bool {
  flag := sendUserMsg(msgUrl, "case7")
  if !flag {
    return false
  }
  flag = getUserOfflineMsg(offlineUrl)
  if !flag {
    return false
  }
  return true
}

func testCase12(subUrl string, conn net.Conn, client *ClientInfo) bool {
  response, err := http.Get(subUrl)
  if err != nil {
    fmt.Println("get offline msg is error,err:%v", err)
    return false
  }
  defer response.Body.Close()
  //fmt.Println("suburl:%s,resp:%v", subUrl, response)
  return true
}

func exitTest(client *ClientInfo) {
  client.Status = MsgTypeClosed
}

func handleClient(index int, checkServerAddr string, userName string, offlineUrl, msgUrl, channelUrl, subUrl, unsubUrl string, client *ClientInfo, otherChannel, otherSub, otherUnsub string) {
  defer exitTest(client)
  fmt.Println("begin test server, client:" + userName)
  // case 1 connect server, login
  conn, flag := testCase1(checkServerAddr, userName, client)
  if !flag {
    fmt.Println("case1 fail,client:" + userName)
    return
  }
  fmt.Println("case1 pass,client:" + userName)

  // case 2 online msg
  // case 3: push msg
  // case 4: ack
  client.Status = MsgTypeMsg
  go recvMsgAndSendAck(conn, client)

  flag = testCase2(conn, client, msgUrl)
  if !flag {
    fmt.Println("case2 fail,client:" + userName)
    exit(conn, client)
    return
  }
  fmt.Println("case2 pass,client:" + userName)
  time.Sleep(5 * time.Second)
  flag = getUserOfflineMsg(offlineUrl)
  if flag {
    fmt.Println("case3 fail,client:" + userName)
    return
  }
  fmt.Println("case3 pass,client:" + userName)
  fmt.Println("case4 pass,client:" + userName)
  // case 5
  flag = testCase5(conn, client, msgUrl)
  if !flag {
    fmt.Println("case5 fail,client:" + userName)
    return
  }
  time.Sleep(5 * time.Second)
  flag = getUserOfflineMsg(offlineUrl)
  if flag {
    fmt.Println("case5 fail,client:" + userName)
    return
  }
  fmt.Println("case5 pass,client:" + userName)
  conn.Close()
  fmt.Println("case6 pass,client:" + userName)
  time.Sleep(5 * time.Second)
  flag = testCase7(msgUrl, offlineUrl)
  if !flag {
    fmt.Println("case7 fail,client:" + userName)
    return
  }
  fmt.Println("case7 pass,client:" + userName)
  client.Status = MsgTypeLogin
  conn = connectServer(checkServerAddr)
  if conn == nil {
    fmt.Println("connect server is error, addr:%v", checkServerAddr)
    return
  }
  go recvMsgAndSendAck(conn, client)
  buf := []byte("GET /connect?uid=" + userName + "&password=" + userName + " HTTP/1.1\r\nUser-Agent: mobile_socket_client/0.1.0\r\nAccept: */*\r\n\r\n")
  flag = writeMsg(buf, conn)
  if !flag {
    fmt.Println("case8 fail,client:" + userName)
    return
  }
  fmt.Println("case8 pass,client:" + userName)
  time.Sleep(5 * time.Second)
  flag = getUserOfflineMsg(offlineUrl)
  if flag {
    fmt.Println("case9 fail,client:" + userName)
    return
  }
  fmt.Println("case9 pass,client:" + userName)
  flag = testCase5(conn, client, msgUrl)
  if !flag {
    fmt.Println("case10 fail,client:" + userName)
    return
  }
  time.Sleep(5 * time.Second)
  flag = getUserOfflineMsg(offlineUrl)
  if flag {
    fmt.Println("case10 fail,client:" + userName)
    return
  }
  fmt.Println("case10 pass,client:" + userName)
  // get channelurl
  go recvMsgAndSendAck(conn, client)
  flag = testCase12(subUrl, conn, client)
  if !flag {
    fmt.Println("case11 fail,client:" + userName)
    return
  }
  time.Sleep(2 * time.Second)
  fmt.Println("case11 pass,client:" + userName)
  flag = sendUserMsg(channelUrl, "case12")
  if !flag {
    fmt.Println("case 12 fail,client:" + userName)
    exit(conn, client)
    return
  }
  time.Sleep(5 * time.Second)
  flag = getUserOfflineMsg(offlineUrl)
  if !flag { // only one
    fmt.Println("case12 fail,client:" + userName)
    return
  }
  fmt.Println("case12 pass,client:" + userName)
  // case 13 disconnect
  conn.Close()
  time.Sleep(2 * time.Second)
  // case 14

  flag = sendUserMsg(channelUrl, "case13")
  if !flag {
    fmt.Println("case13 fail,client:" + userName)
    return
  }
  fmt.Println("case13 pass,client:" + userName)
  conn = connectServer(checkServerAddr)
  if conn == nil {
    fmt.Println("connect server is error, addr:%v", checkServerAddr)
    return
  }
  client.Status = MsgTypeLogin
  go recvMsgAndSendAck(conn, client)
  buf = []byte("GET /connect?uid=" + userName + "&password=" + userName + " HTTP/1.1\r\nUser-Agent: mobile_socket_client/0.1.0\r\nAccept: */*\r\n\r\n")
  flag = writeMsg(buf, conn)
  if !flag {
    fmt.Println("case14 login fail,client:" + userName)
    return
  }
  //go recvMsgAndSendAck(conn, client)
  time.Sleep(5 * time.Second)
  flag = getUserOfflineMsg(offlineUrl)
  if !flag {
    fmt.Println("case14 get offline fail,client:" + userName)
    return
  }
  fmt.Println("case14 pass,client:" + userName)
  conn.Close()
  conn = connectServer(checkServerAddr)
  if conn == nil {
    fmt.Println("connect server is error, addr:%v", checkServerAddr)
    return
  }
  client.Status = MsgTypeLogin
  go recvMsgAndSendAck(conn, client)
  buf = []byte("GET /connect?uid=" + userName + "&password=" + userName + " HTTP/1.1\r\nUser-Agent: mobile_socket_client/0.1.0\r\nAccept: */*\r\n\r\n")
  flag = writeMsg(buf, conn)
  if !flag {
    fmt.Println("case14 login fail,client:" + userName)
    return
  }
  time.Sleep(5 * time.Second)
  conn.Close()
  time.Sleep(5 * time.Second)
  conn = connectServer(checkServerAddr)
  if conn == nil {
    fmt.Println("connect server is error, addr:%v", checkServerAddr)
    return
  }
  client.Status = MsgTypeLogin
  go recvMsgAndSendAck(conn, client)
  buf = []byte("GET /connect?uid=" + userName + "&password=" + userName + " HTTP/1.1\r\nUser-Agent: mobile_socket_client/0.1.0\r\nAccept: */*\r\n\r\n")
  flag = writeMsg(buf, conn)
  if !flag {
    fmt.Println("case14 login fail,client:" + userName)
    return
  }
  //fmt.Println("begin test different shard")
  flag = optChannelMsg(unsubUrl)
  if !flag {
    fmt.Println("case15 unsub fail,client:" + userName)
    return
  }

  time.Sleep(10 * time.Second)
  flag = sendUserMsg(channelUrl, "case15")
  if !flag {
    fmt.Println("case15 channel fail,client:" + userName)
    return
  }

  flag = getUserOfflineMsg(offlineUrl)
  if flag {
    fmt.Println("case15 get offline fail,client:" + userName)
    return
  }
  fmt.Println("case15 pass,client:" + userName)
  flag = optChannelMsg(otherSub)
  if !flag {
    fmt.Println("case16 fail,client:" + userName)
    return
  }
  fmt.Println("case16 pass,client:" + userName)
  go recvMsgAndSendAck(conn, client)
  flag = sendUserMsg(otherChannel, "case17")
  if !flag {
    fmt.Println("case17 fail,client:" + userName)
    return
  }
  time.Sleep(5 * time.Second)
  flag = getUserOfflineMsg(offlineUrl)
  if flag {
    fmt.Println("case17 fail,client:" + userName)
    return
  }
  fmt.Println("case17 pass,client:" + userName)
  conn.Close()
  flag = sendUserMsg(otherChannel, "case18")
  if !flag {
    fmt.Println("case18 fail,client:" + userName)
    return
  }
  flag = getUserOfflineMsg(offlineUrl)
  if !flag {
    fmt.Println("case18 fail,client:" + userName)
    return
  }
  fmt.Println("case18 pass,client:" + userName)
  conn = connectServer(checkServerAddr)
  if conn == nil {
    fmt.Println("connect server is error, addr:%v", checkServerAddr)
    return
  }
  client.Status = MsgTypeLogin
  go recvMsgAndSendAck(conn, client)
  buf = []byte("GET /connect?uid=" + userName + "&password=" + userName + " HTTP/1.1\r\nUser-Agent: mobile_socket_client/0.1.0\r\nAccept: */*\r\n\r\n")
  flag = writeMsg(buf, conn)
  if !flag {
    fmt.Println("case19 fail,client:" + userName)
    return
  }
  time.Sleep(5 * time.Second)
  flag = getUserOfflineMsg(offlineUrl)
  if flag {
    fmt.Println("case19 fail,client:" + userName)
    return
  }
  fmt.Println("case19 pass,client:" + userName)
  flag = optChannelMsg(otherUnsub)
  if !flag {
    fmt.Println("case20 fail,unsub, client:" + userName)
    return
  }
  //go sendSingleMsg(msgUrl)
  flag = optChannelMsg(client.Shard.SubUrl)
  if !flag {
    fmt.Println("case20 fail,client:" + userName)
    return
  }
  conn.Close()
  time.Sleep(2 * time.Second)
  // send channel msg from channel1
  flag = verifyTtl(msgUrl+"&ttl=5", offlineUrl, "case20")
  if !flag {
    fmt.Println("case20 verify fail,client:" + userName)
    return
  }
  fmt.Println("case20 pass,client:" + userName)
  flag = verifyTtl(client.Shard.MsgUrl+"&ttl=5", offlineUrl, "case21")
  if !flag {
    fmt.Println("case21 verify fail,client:" + userName)
    return
  }
  fmt.Println("case21 pass,client:" + userName)
  fmt.Println("case22 pass,client:" + userName)

  flag = verifyTtl(otherChannel+"&ttl=5", offlineUrl, "case23")
  if !flag {
    fmt.Println("case23 verify fail,client:" + userName)
    return
  }
  fmt.Println("case23 pass,client:" + userName)
  flag = verifyTtl(client.Shard.ChannelUrl+"&ttl=5", offlineUrl, "case24")
  if !flag {
    fmt.Println("case24 verify fail,client:" + userName)
    return
  }
  fmt.Println("case24 pass,client:" + userName)

  flag = optChannelMsg(client.Shard.UnsubUrl)
  if !flag {
    fmt.Println("case25 fail,client:" + userName)
    return
  }
  time.Sleep(5 * time.Second)
  flag = sendUserMsg(otherChannel, "case25")
  if !flag {
    fmt.Println("case25 fail,client:" + userName)
    return
  }
  flag = getUserOfflineMsg(offlineUrl)
  if flag {
    fmt.Println("case25 fail,client:" + userName)
    return
  }
  fmt.Println("case25 pass,client:" + userName)
  fmt.Println("test end,all pass,client:" + userName)
  client.Status = MsgTypeClosed
}

func verifyTtl(msgUrl2, offlineUrl, caseText string) bool {
  flag := sendUserMsg(msgUrl2, "offlinemsg")
  time.Sleep(2 * time.Second)
  flag = verifyUserOfflineMsg(offlineUrl)
  if !flag {
    //fmt.Println("%s fail get", caseText)
    return false
  }
  time.Sleep(5 * time.Second)
  flag = verifyUserOfflineMsg(offlineUrl)
  if flag {
    //fmt.Println("%s fail ", caseText)
    return false
  }
  //fmt.Println("%s pass", caseText)
  return true
}

func recvMsgAndSendAck(conn net.Conn, client *ClientInfo) {
  buf := recvMsgRoutine(conn, client)
  recvFlag := handleMsg(buf, client)
  if recvFlag {
    toSendAckMsg := buildMsg(MsgTypeAck, client)
    if g_log_switch {
      fmt.Println("-----" + string(buf))
      fmt.Println("-----" + toSendAckMsg)
    }
    flag := sendMsgRoutine(conn, client, []byte(toSendAckMsg))
    if !flag {
      return
    }
    client.AckSuccId += 1
  }
  return
}

func sendMsg(msgUrl string, client *ClientInfo, conn net.Conn) bool {
  flag := sendUserMsg(msgUrl, "sendmsg")
  if !flag {
    return false
  }
  return flag
}

func sendLoginMsg(conn net.Conn, userName string, client *ClientInfo) bool {
  buf := []byte("GET /connect?uid=" + userName + "&password=" + userName + " HTTP/1.1\r\nUser-Agent: mobile_socket_client/0.1.0\r\nAccept: */*\r\n\r\n")
  flag := writeMsg(buf, conn)
  if !flag {
    return false
  }

  flag = recvLoginRespMsg(conn, client)
  return flag
}

func recvLoginRespMsg(conn net.Conn, client *ClientInfo) bool {
  // read head
  reader := bufio.NewReader(conn)
  return testrecv(reader, client)
}

func testrecv(reader *bufio.Reader, client *ClientInfo) bool {
  var flag bool
  var line string

  //conn.SetReadDeadline(time.Now().Add(2 * time.Minute))
  for flag != true {
    linePrefix, err := reader.ReadString('\n')
    if err != nil {
      if err == io.EOF {
        return false
      }
      e, ok := err.(net.Error)
      if !ok || !e.Temporary() {
        fmt.Println("user exit, err:%v", err)
        return false
      }
      fmt.Println("ok user exit, err:%v", err)
      return false
    }
    if len(line) == 0 {
      line = linePrefix
    }

    flag = strings.Contains(linePrefix, "\r\n")
    if flag {
      lineAftfix, err := reader.ReadString('\n')
      if err != nil {
        if err == io.EOF {
          return false
        }
        e, ok := err.(net.Error)
        if !ok || !e.Temporary() {
          fmt.Println("flag user exit, err:%v", err)
          return false
        }
        fmt.Println("flag ok user exit, err:%v", err)
        return false
      }
      if string(lineAftfix) == "\r\n" {
        if strings.Contains(line, LoginRespVerify) {
          return true
        } else {
          return false
        }
      }
      flag = false
    }
  }
  return false
}

func buildMsg(msgType string, client *ClientInfo) string {
  var content string
  switch msgType {
  case MsgTypeAck:
    /*
       ackMsg := &Ack{}
       ackMsg.Type = msgType
       ackMsg.From = client.UserName
       ackMsg.Seq = client.Seq
       content, _ = json.Marshal(ackMsg)
    */
    content = `{"y": 5, "f": "` + client.UserName + `", "s":`
    content = fmt.Sprintf(content+"%d}", client.Seq)
  case MsgTypeHeartBeat:
    content = `{"y":0}`
  }

  header := strconv.FormatInt(int64(len(content)), 16)

  return (header + "\r\n" + content + "\r\n")
}

func handleMsg(buf []byte, client *ClientInfo) bool {
  msg := &Msg{}
  err := json.Unmarshal(buf, msg)
  if err != nil {
    fmt.Println("%s user unmarshal msg error,err:%v, buf:%s", client.UserName, err, string(buf))
    return false
  }
  seqId := client.Seq
  if seqId >= msg.Seq {
    client.ErrMsgId = client.ErrMsgId + 1
  } else if msg.Seq-seqId == 1 {
    client.SuccMsgId = client.SuccMsgId + 1
  } else {
    client.WarnMsgId = client.WarnMsgId + 1
  }
  client.Seq = msg.Seq
  client.MsgId = client.MsgId + 1
  if client.Case == "case3" {
    fmt.Println("case 3 pass")
    client.Case = "case4"
  }
  return true
}

func exit(conn net.Conn, client *ClientInfo) {
  client.BreakLoop = true
  conn.Close()
  client.UnConnectId += 1
  //close(client.ExitCh)
  //close(client.RecvCh)
  //close(client.SendCh)
}

func sendMsgRoutine(conn net.Conn, client *ClientInfo, buf []byte) bool {
  //fmt.Println("sendData:%s", sendData)
  sendFlag := writeMsg(buf, conn)
  if !sendFlag {

    return false
  }
  return true
}

func recvMsgRoutine(conn net.Conn, client *ClientInfo) []byte {
  reader := bufio.NewReader(conn)
  for {
    if client.Status == MsgTypeLogin {
      flag := testrecv(reader, client)
      if !flag {
        fmt.Println("recv login msg is error")
        return nil
      }
      client.Status = MsgTypeMsg
    }

    line, err := reader.ReadString('\n')
    if err != nil {
      if err == io.EOF {
        return nil
      }

      fmt.Println("user(%s), read msg flag is error,err:%v", client.UserName, err)
      return nil
    }

    if !strings.Contains(line, "\r\n") {
      return nil
    }
    prefixArr := strings.Split(line, "\r")
    if len(prefixArr) != 2 {
      fmt.Println("user %s read header error, header:%v", client.UserName, line)
      return nil
    }

    toReadLen, _ := strconv.ParseUint(prefixArr[0], 16, 32)
    // read body
    bodyLen := 0
    readLen := int(toReadLen)
    buf := make([]byte, readLen)
    for bodyLen < readLen {
      tmpLen, err := reader.Read(buf[bodyLen:])
      if err != nil {
        if err == io.EOF {
          //fmt.Println("%d user closed,op:%s", index, msgType)
          return nil
        }
        fmt.Println("user %s recv data error,err:%v", client.UserName, err)
        return nil
      }
      bodyLen += tmpLen
    }
    verify, err := reader.ReadString('\n')
    if err != nil {
      if err == io.EOF {
        return nil
      }
      fmt.Println("user(%s) read after flag is error,err;%v", client.UserName, err)
      return nil
    }
    if string(verify) != "\r\n" {
      fmt.Println("user verify msg:%s,op:%s", client.UserName, string(verify))
      return nil
    }
    return buf
  }
  return nil
}

func writeMsg(buf []byte, conn net.Conn) bool {
  var sendLen int
  var sendErrCn int
  msgLen := len(buf)
  for sendLen < msgLen {
    tmpLen, err := conn.Write(buf[sendLen:])
    if err != nil {
      if err == io.EOF {
        return false
      }
      if sendErrCn < MaxSendMsgErrCount {
        sendErrCn += 1
        continue
      }
      //fmt.Println("%d user %s error,err:%v", index, msgType, err)
      return false
    }
    sendLen += tmpLen
  }

  return true
}
