# cpp-zmq-bookstore-demo
C++ + ZeroMQ 书店销售统计演示项目
采用 PUSH-PULL 消息模型，实现多线程并发统计销售数据；集成Lua动态配置、Msgpack二进制序列化、定时HTTP指标上报。

## 依赖环境（Ubuntu系统）
```bash
sudo apt install libzmq3-dev luajit libcurl-dev
```
组件说明：
ZeroMQ：消息队列，客户端与服务端通信
LuaJIT + LuaBridge：加载外部配置 conf.lua
cpr/curl：HTTP 接口上报统计数据
msgpack-cxx：业务数据序列化

编译流程
克隆代码
```bash
git clone https://github.com/WenjunXueCPP/cpp-zmq-bookstore-demo.git
cd cpp-zmq-bookstore-demo
```

创建编译目录构建项目
```bash
mkdir build
cd build
cmake ..
make -j4
```

启动运行
建议项目根目录运行程序，配置文件conf.lua可直接加载，无需复制文件。
终端 1（启动服务端）
```bash
./build/srv
```
终端 2（启动客户端，模拟门店推送销售订单）
```bash
./build/client
```

目录代码说明
srv.cpp：服务端，接收消息、多线程聚合统计销售信息
client.cpp：生产者客户端，模拟发送销售数据
SalesData.hpp：销售数据结构体，提供序列化 / 反序列化
Summary.hpp：线程安全的数据汇总模块
SpinLock.hpp：自研自旋锁实现
Zmq.hpp：ZeroMQ 简易封装
Config.hpp：Lua 配置文件读取封装
conf.lua：项目外部配置

额外备注
build 为本地编译生成文件夹，通过.gitignore 忽略，不上传仓库；
运行出现http post failed属于正常现象：本地没有部署对应的 http 接收服务，不影响 ZMQ 消息收发核心功能；
使用 Ctrl + C 终止程序。
