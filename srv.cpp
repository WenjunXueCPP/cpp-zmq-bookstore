// 订单通信服务端
// 收单：ZeroMQ PULL → 线程池并发解码 → 按商品 ID 聚合
// 上报：独立线程每 N 秒把统计结果 JSON POST 到业务接口
// 监控：独立线程每秒打印一次 QPS，压测时直接读数
//
// 基于课程示例代码改造，三处改动：
//   1. 修复 msgpack 反序列化对象生命周期问题（object_handle 提前析构导致悬空引用）
//   2. 每条消息 detach 一个线程 → 固定大小线程池，线程数可控、有上限
//   3. 去掉每条消息的 stdout 打印，改为每秒汇总输出

#include "cpplang.hpp"
#include "Summary.hpp"
#include "Zmq.hpp"
#include "Config.hpp"
#include "ThreadPool.hpp"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <cstdio>
#include <cpr/cpr.h>

USING_NAMESPACE(std);
USING_NAMESPACE(cpp_study);

int main()
try
{
    cout << "order server start" << endl;

    Config conf;
    conf.load("./conf.lua");

    Summary sum;
    std::atomic_int count {0};
    std::atomic_int recv_count {0};

    // 线程池大小：conf.lua 里配置，读不到就按 CPU 核数
    int thread_num = 0;
    try {
        thread_num = conf.get<int>("config.thread_num");
    }
    catch (std::exception&) {
        thread_num = 0;
    }

    if (thread_num <= 0) {
        thread_num = static_cast<int>(std::thread::hardware_concurrency());
        if (thread_num <= 0) {
            thread_num = 4;
        }
    }

    ThreadPool pool(static_cast<std::size_t>(thread_num));
    cout << "thread pool size = " << pool.worker_count() << endl;

    // 接收循环：只负责取消息、投递任务，不做任何耗时操作
    auto recv_cycle = [&]()
    {
        using zmq_ctx = ZmqContext<1>;

        auto sock = zmq_ctx::recv_sock();

        sock.bind(conf.get<string>("config.zmq_ipc_addr"));
        assert(sock.connected());

        for (;;) {
            auto msg_ptr = std::make_shared<zmq_message_type>();

            sock.recv(msg_ptr.get());

            ++recv_count;

            pool.submit([&sum, &count, msg_ptr]()
            {
                SalesData book;

                // 关键：object_handle 必须活到 convert 结束
                msgpack::object_handle oh =
                    msgpack::unpack(msg_ptr->data<char>(), msg_ptr->size());

                oh.get().convert(book);

                sum.add_sales(book);

                ++count;
            });
        }
    };

    // 每秒打印一次吞吐，压测读数用
    auto monitor_cycle = [&]()
    {
        int last = 0;

        for (;;) {
            std::this_thread::sleep_for(1s);

            int now = count.load();
            cout << "[qps] " << (now - last)
                 << "  done " << now << "  recv " << recv_count.load() << endl;

            last = now;
        }
    };

    // 定时上报：每 time_interval 秒 POST 一次累计计数与最值商品
    auto log_cycle = [&]()
    {
        auto http_addr     = conf.get<string>("config.http_addr");
        auto time_interval = conf.get<int>("config.time_interval");

        bool first_fail = true;

        for (;;) {
            std::this_thread::sleep_for(time_interval * 1s);

            json j;
            j["count"]  = static_cast<int>(count);
            j["minmax"] = sum.minmax_sales();

            auto res = cpr::Post(
                        cpr::Url{http_addr},
                        cpr::Body{j.dump()},
                        cpr::Timeout{200ms});

            if (res.status_code != 200 && first_fail) {
                cerr << "http post failed (only printed once)" << endl;
                first_fail = false;
            }
        }
    };

    auto fu1 = std::async(std::launch::async, log_cycle);
    auto fu2 = std::async(std::launch::async, monitor_cycle);
    auto fu3 = std::async(std::launch::async, recv_cycle);

    fu3.wait();

    return 0;
}
catch (std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return 1;
}
