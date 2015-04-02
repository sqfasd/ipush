package main

import (
  "bufio"
  "bytes"
  l4g "code.google.com/p/log4go"
  "encoding/json"
  "flag"
  "fmt"
  "io"
  "io/ioutil"
  "net"
  "net/http"
  "os"
  //"os/signal"
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

type Ack struct {
  Type string `json:"type"`
  From string `json:"from"`
  Seq  int64  `json:"seq"`
}

type Msg struct {
  Type string `json:"type"`
  To   string `json:"to"`
  Body string `json:"body"`
  Seq  int64  `json:"seq"`
}

type HeartBeatMsg struct {
  Type string `json:"type"`
}

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
}

var Log l4g.Logger

func init() {
  Log = make(l4g.Logger)
  Log.LoadConfiguration("conf.xml")
}

var clientInfoMap map[string]*ClientInfo
var localVirPort int
var localVirIp string

func main() {
  defer func() { //必须要先声明defer，否则不能捕获到panic异常
    if err := recover(); err != nil {
      Log.Error("err:%v", err) //这里的err其实就是panic传入的内容
    }
  }()
  numCpu := runtime.NumCPU()
  runtime.GOMAXPROCS(numCpu)
  checkServerAddr := flag.String("r", "127.0.0.1:9100", "to do check server addr,eg:127.0.0.1:9100")
  localAddr := flag.String("l", "127.0.0.1:9100", "local addr,eg:127.0.0.1:9100")
  clientNum := flag.Int("n", 10000, "produce client num")
  userName := flag.String("u", "", "user name prefix")
  port := flag.String("p", "", "listen port")
  flag.Parse()

  Log.Error("start check server,addr:%s,cpu:%d,clientNum:%d, port:%s, userNamePrefix:%s", *checkServerAddr, numCpu, *clientNum, *port, *userName)
  clientInfoMap = make(map[string]*ClientInfo, *clientNum)
  localArr := strings.Split(*localAddr, ":")

  localVirIp = localArr[0]
  localVirPort, _ = strconv.Atoi(localArr[1])
  var i int
  for ; i < *clientNum; i += 1 {
    client := &ClientInfo{}
    client.RecvCh = make(chan string)
    client.SendCh = make(chan string)
    client.ExitCh = make(chan string)
    client.UserName = *userName + "_" + strconv.Itoa(i)
    client.Pass = client.UserName
    client.TimeUnix = time.Now().Unix()
    //client.LoginMsg = "GET /connect?uid=" + client.UserName + "&password=" + client.Pass + " HTTP/1.1\r\nUser-Agent: mobile_socket_client/0.1.0\r\nAccept: */*\r\n\r\n"
    clientInfoMap[client.UserName] = client
    go handleClient(i, *checkServerAddr, client)
    if i%1000 == 0 {
      time.Sleep(10 * time.Second)
    }
  }
  http.HandleFunc("/stats", statsClientInfo)
  http.ListenAndServe(localVirIp+":"+*port, nil)
}

func statsClientInfo(w http.ResponseWriter, r *http.Request) {
  body, err := ioutil.ReadAll(r.Body)
  if err != nil {
    Log.Error("accept order error, err:%v", err)
    w.Write([]byte("order is error!"))
    return
  }
  msg := &OrderMsg{}
  err = json.Unmarshal(body, msg)
  if err != nil {
    Log.Error("order unmarshal error,body:%s, err:%v", string(body), err)
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
    Log.Error("write to file is error, err:%s" + err.Error())
    return
  }
  w.Flush()
}

func sendLoginMsg(conn net.Conn, index int, msgType string, client *ClientInfo) bool {
  buf := []byte("GET /connect?uid=" + client.UserName + "&password=" + client.Pass + " HTTP/1.1\r\nUser-Agent: mobile_socket_client/0.1.0\r\nAccept: */*\r\n\r\n")
  flag := writeMsg(buf, conn)
  if !flag {
    return false
  }
  flag = recvLoginRespMsg(conn, index, msgType)
  return flag
}

func recvLoginRespMsg(conn net.Conn, index int, msgType string) bool {
  // read head
  var flag bool
  reader := bufio.NewReader(conn)
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
        Log.Error("%d user exit, err:%v", index, err)
        return false
      }
      Log.Error("%d ok user exit, err:%v", index, err)
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
          Log.Error("%d flag user exit, err:%v", index, err)
          return false
        }
        Log.Error("%d flag ok user exit, err:%v", index, err)
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

func exit(conn net.Conn, client *ClientInfo) {
  client.BreakLoop = true
  conn.Close()
  client.UnConnectId += 1
  close(client.ExitCh)
  close(client.RecvCh)
  close(client.SendCh)
}

func handleClient(index int, checkServerAddr string, client *ClientInfo) {
  var conn net.Conn
  var err error
  var localaddr net.TCPAddr
  var remoteaddr net.TCPAddr
  localaddr.IP = net.ParseIP(localVirIp)
  localaddr.Port = localVirPort
  remoteArr := strings.Split(checkServerAddr, ":")
  remoteaddr.IP = net.ParseIP(remoteArr[0])

  remoteaddr.Port, err = strconv.Atoi(remoteArr[1])
  if err != nil {
    Log.Error("err:%v", err)
    return
  }

  conn, err = net.DialTCP("tcp", &localaddr, &remoteaddr)

  if err != nil {
    Log.Error("%v connect server error,err:%v", localaddr, err)
    return
  }

  client.Conn = conn
  statusFlag := sendLoginMsg(conn, index, MsgTypeLogin, client)
  if statusFlag {
    client.LoginStatusId += 1
    client.Status = MsgTypeMsg
  } else {
    client.LoginErrId += 1
    exit(conn, client)
    return
  }
  go sendMsgRoutine(conn, client)
  go recvMsgRoutine(conn, client)
  for {
    select {
    case <-time.After(ConnTimeOutNum):
      if clientInfoMap[client.UserName].Status != MsgTypeLogin {
        // send heartbeat
        toSendHbMsg := buildMsg(MsgTypeHeartBeat, client)
        client.SendCh <- toSendHbMsg
        client.SendHeartBeatRecvSuccId += 1
      }
    case recvData := <-client.RecvCh:
      if client.Status == MsgTypeMsg {
        recvFlag := handleMsg([]byte(recvData), client)
        if recvFlag {
          toSendAckMsg := buildMsg(MsgTypeAck, client)
          client.SendCh <- toSendAckMsg
          client.AckSuccId += 1
        } else {
          client.JsonErr += 1
        }
      } else {
        Log.Error("user %s status(%s) is error!", client.UserName, client.Status)
        return
      }
    case <-client.ExitCh:
      client.BreakLoop = true
      return
    }
  }
}

func buildMsg(msgType string, client *ClientInfo) string {
  var content []byte
  switch msgType {
  case MsgTypeAck:
    ackMsg := &Ack{}
    ackMsg.Type = msgType
    ackMsg.From = client.UserName
    ackMsg.Seq = client.Seq
    content, _ = json.Marshal(ackMsg)
  case MsgTypeHeartBeat:
    hb := &HeartBeatMsg{}
    hb.Type = msgType
    content, _ = json.Marshal(hb)
  }

  strContent := string(content)
  header := strconv.FormatInt(int64(len(strContent)), 16)

  return (header + "\r\n" + strContent + "\r\n")
}

func handleMsg(buf []byte, client *ClientInfo) bool {
  msg := &Msg{}
  err := json.Unmarshal(buf, msg)
  if err != nil {
    Log.Error("%s user unmarshal msg error,err:%v, buf:%s", client.UserName, err, string(buf))
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
  return true
}

func sendMsgRoutine(conn net.Conn, client *ClientInfo) {
  for {
    select {
    case sendData := <-client.SendCh:
      //Log.Error("sendData:%s", sendData)
      sendFlag := writeMsg([]byte(sendData), conn)
      if !sendFlag {
        client.ExitCh <- ConnClosedMsg
        return
      }
    }

    if client.BreakLoop {
      exit(conn, client)
      return
    }
  }
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
      //Log.Error("%d user %s error,err:%v", index, msgType, err)
      return false
    }
    sendLen += tmpLen
  }
  return true
}

func recvMsgRoutine(conn net.Conn, client *ClientInfo) {
  // read head
  reader := bufio.NewReader(conn)
  for {
    line, err := reader.ReadString('\n')
    if err != nil {
      if err == io.EOF {
        client.ExitCh <- ConnClosedMsg
        return
      }
      client.ExitCh <- ConnClosedMsg
      Log.Error("user(%s), read msg flag is error,err:%v", client.UserName, err)
      return
    }

    if !strings.Contains(line, "\r\n") {
      client.ExitCh <- ConnClosedMsg
      return
    }
    prefixArr := strings.Split(line, "\r")
    if len(prefixArr) != 2 {
      Log.Error("user %s read header error, header:%v", client.UserName, line)
      client.ExitCh <- ConnClosedMsg
      return
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
          //Log.Error("%d user closed,op:%s", index, msgType)
          client.ExitCh <- ConnClosedMsg
          return
        }
        Log.Error("user %s recv data error,err:%v", client.UserName, err)
        client.ExitCh <- ConnClosedMsg
        return
      }
      bodyLen += tmpLen
    }
    verify, err := reader.ReadString('\n')
    if err != nil {
      if err == io.EOF {
        client.ExitCh <- ConnClosedMsg
        return
      }
      Log.Error("user(%s) read after flag is error,err;%v", client.UserName, err)
      client.ExitCh <- ConnClosedMsg
      return
    }
    if string(verify) != "\r\n" {
      Log.Error("user verify msg:%s,op:%s", client.UserName, string(verify))
      client.ExitCh <- ConnClosedMsg
      return
    }
    if client.BreakLoop {
      exit(conn, client)
      return
    }
    client.RecvCh <- string(buf)
  }
}
