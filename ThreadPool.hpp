// 固定大小线程池：任务队列 + 条件变量，替代"每条消息 detach 一个线程"
#ifndef _THREAD_POOL_HPP
#define _THREAD_POOL_HPP

#include "cpplang.hpp"

#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>

BEGIN_NAMESPACE(cpp_study)

class ThreadPool final
{
public:
    explicit ThreadPool(std::size_t n)
        : m_stop(false)
    {
        for (std::size_t i = 0; i < n; ++i) {
            m_workers.emplace_back([this] { worker_loop(); });
        }
    }

    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lk(m_mtx);
            m_stop = true;
        }
        m_cv.notify_all();

        for (auto& t : m_workers) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

public:
    void submit(std::function<void()> task)
    {
        {
            std::unique_lock<std::mutex> lk(m_mtx);
            m_tasks.push(std::move(task));
        }
        m_cv.notify_one();
    }

    std::size_t worker_count() const noexcept
    {
        return m_workers.size();
    }

private:
    void worker_loop()
    {
        for (;;) {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lk(m_mtx);
                m_cv.wait(lk, [this] { return m_stop || !m_tasks.empty(); });

                if (m_stop && m_tasks.empty()) {
                    return;
                }

                task = std::move(m_tasks.front());
                m_tasks.pop();
            }

            // 单条消息解码失败不能拖垮整个线程
            try {
                task();
            }
            catch (std::exception& e) {
                std::cerr << "task error: " << e.what() << std::endl;
            }
        }
    }

private:
    std::vector<std::thread>        m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex                      m_mtx;
    std::condition_variable         m_cv;
    bool                            m_stop;
};

END_NAMESPACE(cpp_study)

#endif  //_THREAD_POOL_HPP
