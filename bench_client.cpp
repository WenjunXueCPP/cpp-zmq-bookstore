// 多线程压测客户端：N 个线程各自建 PUSH socket，轮转发预生成的订单报文
// 用法：./bench_client [线程数] [每线程条数]
//   ./bench_client 8 20000     → 8 线程 × 2 万条 = 16 万条
//
// 说明：PUSH/PULL 是单向管道，客户端收不到服务端确认，所以这里只报"发送侧速率"，
//      真正的服务端处理 QPS 看服务端每秒打印的 [qps] 那一行。

#include "cpplang.hpp"
#include "SalesData.hpp"
#include "Zmq.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>

USING_NAMESPACE(std);
USING_NAMESPACE(cpp_study);

int main(int argc, char** argv)
{
    const char* addr = "tcp://127.0.0.1:5555";

    int threads    = (argc > 1) ? std::atoi(argv[1]) : 8;
    int per_thread = (argc > 2) ? std::atoi(argv[2]) : 20000;

    if (threads <= 0 || threads > 128) {
        std::cerr << "threads must be 1..128" << std::endl;
        return 1;
    }
    if (per_thread <= 0) {
        std::cerr << "per_thread must be > 0" << std::endl;
        return 1;
    }

    long total = static_cast<long>(threads) * per_thread;

    // 预生成 1000 个不同商品的报文，把序列化开销排除在压测之外，
    // 让测到的数字只反映服务端的接收与聚合能力
    const int kBookCount = 1000;

    std::vector<msgpack::sbuffer> bufs;
    bufs.reserve(kBookCount);

    for (int i = 0; i < kBookCount; ++i) {
        char id[16];
        std::snprintf(id, sizeof(id), "book_%04d", i);
        bufs.push_back(SalesData(id,
                                 10 + static_cast<unsigned>(i % 50),
                                 100.0 + i).pack());
    }

    std::cout << "bench start: threads=" << threads
              << " per_thread=" << per_thread
              << " total=" << total
              << " books=" << kBookCount << std::endl;

    auto t0 = std::chrono::steady_clock::now();

    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(threads));

    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&bufs, per_thread, t, addr]()
        {
            using zmq_ctx = ZmqContext<1>;

            // ZeroMQ socket 不是线程安全的，每个线程必须自己建一个
            auto sock = zmq_ctx::send_sock();

            sock.connect(addr);

            std::size_t sent = 0;

            for (int i = 0; i < per_thread; ++i) {
                auto& b = bufs[static_cast<std::size_t>(i + t) % bufs.size()];

                // HWM 满时 ZeroMQ 会阻塞而不是返回错误，这里不判断返回值；
                // 不同版本的 cppzmq 返回类型不一致，比较反而编不过
                sock.send(b.data(), b.size());
                ++sent;
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    auto t1 = std::chrono::steady_clock::now();

    long ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    if (ms <= 0) {
        ms = 1;
    }

    std::cout << "send done: " << total << " msgs in " << ms << " ms, "
              << (static_cast<double>(total) * 1000.0 / static_cast<double>(ms))
              << " msg/s (client side)" << std::endl;

    // 等服务端把队列里剩余的消息消费完，再看它的 [qps] 输出
    std::cout << "wait 3s for server drain, then read server [qps] lines" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));

    return 0;
}
