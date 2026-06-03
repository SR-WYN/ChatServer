#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

template <typename Derived>
class ThreadPoolBase {
protected:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop{false};

public:
    ThreadPoolBase(size_t threads) {
        for(size_t i = 0; i < threads; ++i)
            workers.emplace_back([this] {
                while(true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] { 
                            return this->stop || !this->tasks.empty(); 
                        });
                        if(this->stop && this->tasks.empty()) return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task(); // 执行任务
                }
            });
    }

    ~ThreadPoolBase() {
        stop = true;
        condition.notify_all();
        for(std::thread &worker : workers)
            worker.join();
    }

    // 通过 CRTP 调用派生类的处理逻辑
    template<typename F>
    void enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace(std::forward<F>(f));
        }
        condition.notify_one();
        // 允许派生类在任务入队后执行额外操作
        static_cast<Derived*>(this)->onTaskEnqueued();
    }

    void onTaskEnqueued() {} // 默认实现，派生类可选重写
};