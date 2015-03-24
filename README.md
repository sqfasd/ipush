# xcomet

基于comet技术的高性能分布式消息推送服务器

## 依赖

```
sudo yum install cmake
sudo yum install boost-devel
```

## 编译

** 默认编译类型为release **

```
./build.sh
BUILD_TYPE=release ./build.sh
BUILD_TYPE=debug ./build.sh
```

### 运行

```
start_ssdb_server
start_session_server
start_router_server
```

### 集群部署
```
// TODO
```


## 使用

### 客户端

客户端向session server发起http请求，请求完毕连接不关闭，服务器可以以chunk的方式持续推送消息
如果客户端使用tcp协议实现，那么客户端也可以随时向服务器发送消息
下面这个方法是普通的http客户端，所以只能单向的接受服务器推送的消息

```
curl http://session_server_host:9000/connect?uid=user001&password=pwd001
```

客户端发送消息的格式
```
{“type”: "sub", "from": "user001", "channel": "channel001"}
{“type”: "unsub", "from": "user001", "channel": "channel001"}
{“type”: "msg", "from": "user001", "to": "user001", "body": "this is a message body"}
{“type”: "ack", "from": "user001", "seq": 1}
```

### 管理员或后端服务

管理员向 router_server 请求向id为user001的用户push数据，该请求类型是 HTTP POST，推送的内容为POST body

```
$ curl -d "@payload" "http://router_server_host:8100/pub?to=user001&from=op"
{
      "result": "ok"
}
```

订阅和取消订阅后端接口

```
$ curl http://router_server_host:8100/sub?uid=user001&cid=channel_id
$ curl http://router_server_host:8100/unsub?uid=user001&cid=channel_id
```

管理员查询服务器状态

```
$ curl "http://session_server_host:9100/stats"
{
   "result" : {
      "server_start_datetime" : "2015/03/23 18:19:44",
      "server_start_timestamp" : 13071579584867722,
      "avg_recv_bytes_per_second" : 2129,
      "avg_recv_number_per_second" : 42,
      "avg_send_bytes_per_second" : 316620,
      "avg_send_number_per_second" : 416,
      "max_recv_bytes_per_second" : 12800,
      "max_recv_number_per_second" : 256,
      "max_send_bytes_per_second" : 7598894,
      "max_send_number_per_second" : 10000,
      "max_user_growth_per_second" : 1,
      "max_user_number" : 1,
      "max_user_reduce_per_second" : 0,
      "total_recv_bytes" : 51103,
      "total_recv_number" : 1024,
      "total_send_bytes" : 7598894,
      "total_send_number" : 10000,
      "user_number" : 1
   }
}
$ curl "http://router_server_host:8100/stats"
{
   "result" : {
      "server_start_datetime" : "2015/03/23 18:19:47",
      "server_start_timestamp" : 13071579587723487,
      "avg_recv_bytes_per_second" : 2629,
      "avg_recv_number_per_second" : 48,
      "avg_send_bytes_per_second" : 361852,
      "avg_send_number_per_second" : 476,
      "max_recv_bytes_per_second" : 15552,
      "max_recv_number_per_second" : 288,
      "max_send_bytes_per_second" : 7598894,
      "max_send_number_per_second" : 10000,
      "max_user_growth_per_second" : 1,
      "max_user_number" : 1,
      "max_user_reduce_per_second" : 0,
      "total_recv_bytes" : 55224,
      "total_recv_number" : 1024,
      "total_send_bytes" : 7598894,
      "total_send_number" : 10000,
      "user_number" : 1
   }
}
```

管理员向 router_server 请求查询某用户的离线消息接口：

```
$ curl "http://master.domain.com:9100/offmsg?uid=1"
{uid: "2", content: ""}
```

## 设计和实现

服务器主要有两类角色：
* router_server: 负责消息的分发，消息的存储， 用户和频道信息汇总 （有些实现称它为broker）
* session_server 负责保持与用户的长连接 （又叫做connector或session manager）

以下将session server简成为 SS， router server简成为RS
当用户连接上SS时，SS发送一个login消息到RS
当用户断开连接时(可能是主动也可能是被动), SS发送一个logout消息到RS
其他消息也是类似，比如订阅频道消息，取消订阅频道消息

无论用户是否在线，推送一条消息时给用户的同时进行持久化
每个用户的消息都有一个序列号max_seq，表示每个用户收到的消息条数
客户端收到消息后会回传一个ack, 并携带收到的消息序列号（不是每个包都会回传ack）
服务器收到ack后即更新last_ack到存储层, 等服务器空闲的时候会删除last_ack之前的所有消息
当用户登陆的时候，系统会取出last_ack到max_seq之间的所有消息(即用户未收到的离线消息）,发送给用户

### 缺陷 & 待改进

* 不保证客户端收到消息的顺序
* 因为客户端不针对所有消息回传ack，假如丢包，并且回传的ack的序列号大于丢的包的序列号，服务器也不会再发送
* 假如router server断开与session server的连接，当重连的时候，session server会将所有的用户重新登陆一遍，这个可能会造成session server的短暂停顿
