# xcomet

## 依赖

+ `libevent`

## 用法

```
mkdir build
cd build
cmake ..
make
```

### 启动 RouterServer

```
make && ./bin/router_server -v=5
```

## API 

### routser_server <=> session_server

router_server 负责：消息的分发，消息的存储，后端的pub请求。

router_server 和 session_server 的交互通过 router_server 主动订阅（rsub） session_server 来交互，接口示例如下：

```
http://slave1.domain.com:9100/rsub
```

router_serer 订阅 session_server ，当 session_server 有转发请求时，通过 http chunk 推送给 router_server 。
rsub 走的协议是 http 的chunk 传输协议，rsub 的服务端传输数据通过 chunk 编码传输，并定期向rsub的请求方发送心跳，
rsub的请求方不主动断开连接，主有当超过一定时间没有接受到心跳请求时，才会断开连接（并尝试重连）。


router_server 和 session_server 交互的协议示例

```
{"type": "login", "uid": "1", "seq": 0}
```

```
{"type": "logout", "uid": "1"}
```

### 客户端 <=> session_server

客户端向session订阅（订阅后就是登陆状态）
uid代表的就是用户id，seq=1是客户端需要维护的一个消息序列号，以便于确保消息没有漏收。

```
http://slave1.domain.com:9000/sub?uid=1&seq=1
```

### 管理员(服务端) <=> router_server 

管理员向 router_server 请求向uid=1的用户 pub数据，该请求是 HTTP POST ，post body 即是发送的数据内容。

```
curl -d "@test.body" "http://master.domain.com:9100/pub?uid=1"
```

管理员向 router_server 请求查询当前在线人数，该接口为 HTTP GET 请求

```
curl "http://master.domain.com:9100/presence"
```

返回结果示例:

```
{"presence": 9678}
```

管理员向 router_server 请求查询某用户的离线消息接口：

```
curl "http://master.domain.com:9100/offmsg?uid=1"
```

返回结果示例：

```
{uid: "2", content: ""}
```

