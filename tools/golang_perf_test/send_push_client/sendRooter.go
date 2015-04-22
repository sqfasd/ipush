package main

import (
  "bytes"
  //l4g "code.google.com/p/log4go"
  "io/ioutil"
  "net/http"
  "strconv"
  "strings"
  "time"
)

/*
var Log l4g.Logger

func init() {
  Log = make(l4g.Logger)
  Log.LoadConfiguration("sendConf.xml")
}
*/
const (
  MaxUserCount = 40000
  MaxSendCount = 10000
  Url          = "http://192.168.2.3:8100/pub?to="
)

type Status struct {
  PostErr    int64
  ReadErr    int64
  UnFoundKey int64
  Succ       int64
}

func main() {
  var ipList = map[string]string{
    "192.168.3.64":  "test_320233",
    "192.168.3.201": "test_3201",
    "192.168.3.200": "test_320212s33",
    "192.168.3.202": "test_3202ddd12ds33",
    "192.168.3.203": "test_320dff3",
    "192.168.3.204": "test_320dddddff3",
    "192.168.1.240": "test_3d2",
    "192.168.1.241": "test_241",
    "192.168.1.242": "test_2d2",
    "192.168.1.32":  "test_321",
    "192.168.1.243": "test_243",
  }
  m := make(map[string]*Status)
  for ip, userRule := range ipList {
    for j := 0; j < MaxSendCount; j += 1 {
      for i := 0; i < MaxUserCount; i += 1 {
        userName := userRule + "_" + strconv.Itoa(i)
        _, ok := m[userName]
        if !ok {
          m[userName] = &Status{}
        }
        body := bytes.NewBuffer([]byte("this is a test message to " + userName))
        res, err := http.Post(Url+userName+"&from=op", "application/json;charset=utf-8", body)
        if err != nil {
          fmt.Println("body:%s,err:%v,user:%s", body, err, userName)
          m[userName].PostErr += 1
          continue
        }
        result, err := ioutil.ReadAll(res.Body)
        if err != nil {
          fmt.Println("read resp err:%v,user:%s", err, userName)
          m[userName].ReadErr += 1
          continue
        }

        if !strings.Contains(string(result), "ok") {
          fmt.Println("ok resp err:%v,user:%s,response:%s", userName, string(result))
          m[userName].UnFoundKey += 1
          continue
        } else {
          m[userName].Succ += 1
        }
      }
      time.Sleep(5 * time.Second)
    }
    fmt.Println("ip:%s end", ip)
  }
  for u, v := range m {
    fmt.Println("totalResult:user:%s:%d:%d:%d:%d", u, v.Succ, v.UnFoundKey, v.ReadErr, v.PostErr)
  }
}
