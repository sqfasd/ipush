GO语言安装步骤:
两种方法：
1. sudo apt-get install golang
2. 直接下载go语言安装包

环境配置：
vi /etc/profile

#go安装的主目录
export GOROOT=/usr/lib/go 
#一般为go语言开发目录 /home/dev/project/src   src下为源码文件
export GOPATH=/home/dev/project/src
export GOBIN=$GOROOT/bin
export PATH=$GOPATH/bin:$PATH
source /etc/profile

