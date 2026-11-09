-- 订单服务配置：改这里不需要重新编译 C++ 代码

config = {

    -- 服务端监听地址，必须与客户端一致
    -- 本机压测用 tcp；跨机部署改成需要绑定的内网 IP
    zmq_ipc_addr = "tcp://127.0.0.1:5555",

    -- 业务数据上报地址；没有对应接口时会打印一次失败提示后静默
    http_addr = "http://localhost/cpp_study?token=cpp@2020",

    -- 上报间隔（秒）
    time_interval = 5,

    -- 处理线程池大小；填 0 或不写则按 CPU 核数
    thread_num = 1,

    keyword = "super",

    sold_criteria = 100,

    revenue_criteria = 1000,

    best_n = 3,

    max_buf_size = 4 * 1024,

}

others = {
    -- add more
}
