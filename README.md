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

router_server <---> session_server 

router_serer 订阅 session_server ，当 session_server 有转发请求时，通过 http chunk 推送给 router_server 。

订阅接口如下：

```
http://session_server_ip:9100/rsub
```

## router_server 和 session_server 交互的协议示例

```
{"type": "user_msg", "to_uid":"1", "content":"hello world"}
```

```
{"type": "login", "uid": "1"}
```

## pub http协议示例

```
```

