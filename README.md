# cpp-zmq-bookstore-demo

ZeroMQ PUSH/PULL 单向订单消息管道：客户端推送订单，服务端线程池并发解码、按商品 ID 聚合，定时把统计结果 POST 到业务接口。

基于 C++ 课程示例改造：修复崩溃、引入线程池、补充压测。

## 改造点

1. **修复 msgpack 悬空引用**：原版 `msgpack::unpack(...).get()` 返回的 object 指向临时 `object_handle` 的 zone，语句结束 zone 即释放，`convert` 读的是已释放内存。改为 `msgpack::object_handle oh = msgpack::unpack(...)`，再 `oh.get().convert(book)`。
2. **线程池替代 detach**：原版每条消息 `std::thread(...).detach()`，无上限。改为固定大小线程池（mutex + condvar + 任务队列），worker 数由 `conf.lua` 的 `thread_num` 决定。
3. **去掉逐条打印**：原版每条消息 `cout << count << endl`。改为独立 monitor 线程每秒输出 `[qps] done recv`。
4. **聚合结构**：`std::map` + 全局自旋锁改为 `unordered_map` + 增量维护 min/max。

## 改造前崩溃

~~~text
order server start
http post failed
terminate called after throwing an instance of 'msgpack::v1::type_error'
  what():  std::bad_cast
~~~

服务端收到第 4 条消息即 abort（use-after-free）。

## 压测结果

环境：VMware Ubuntu 64 位，4 核 / 3 GB，srv 与 bench_client 同机。

| worker | 消息量 | 稳定吞吐 | 处理完毕 |
|---|---|---|---|
| 1 | 160,000 | 9.5 万 msg/s | 约 1.7 s |
| 8 | 800,000 | 25 万 msg/s | 约 3.2 s |

4 核机器上 8 worker 超订 2 倍，加速比 2.6×（65% 扩展效率）。剩余瓶颈：`Summary::add_sales` 的全局自旋锁、`ZmqContext<1>` 单 IO 线程。

口径说明：25 万 msg/s 是消息解码 + 内存聚合吞吐，无 DB、无磁盘、无下游调用。客户端侧 `send` 返回的 msg/s 只反映写入 ZeroMQ 发送缓冲区的速度，不代表服务端处理完成。

## 构建与压测

~~~bash
mkdir -p build && cd build
cmake .. && make -j4
./srv                    # 终端 1
./bench_client 8 100000  # 终端 2，8 线程各 10 万条
~~~

服务端每秒输出：

~~~text
[qps] 253521  done 549387  recv 800000
~~~

`recv - done` 为队列积压量。
