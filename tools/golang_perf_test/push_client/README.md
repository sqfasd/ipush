# push client

模拟真实设备用户，连接服务器，接收服务器发送的push 消息，具体用法如下： 
1.安装go环境
2.编译:
  go build push_client.go
3.执行:
  nohup ./push_client -r 192.168.1.31:9000 -n 40000 -u test_320dddddff3 -p 8880 -l 192.168.3.204:0 &
  -r :push服务器ip端口号
  -n :启动的连接数
  -u :模拟登录用户前缀
  -p :方便查看客户端状态，http服务端口号
  -l :客户端帮定地址
4.查看客户端状态
  curl -d "@testMsg" "http://192.168.3.204:8880/stats"
  testMsg文件如下:
  {
    "type": "stats",
    "userName": "test_320dff3_3070"
  }
  type :stats  查看本机所有客户端状态，userName无意义
        total  查看本机所有客户端状态，将所有用户状态打印到当前目录一个文件中stats_时间戳.dat，userName无意义
        single 查看本机指定userName用户状态，只返回指定用户状态，userName有意义
