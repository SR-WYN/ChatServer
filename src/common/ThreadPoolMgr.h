#pragma once
#include "Singleton.h"
#include "ThreadPool.h"
#include <memory>

class ThreadPoolMgr :public Singleton<ThreadPoolMgr>
{
    friend class Singleton<ThreadPoolMgr>;
public:
    ~ThreadPoolMgr();

    void enqueueLogic(std::function<void()> task);
    // template <typename F, typename... Args>
    // auto enqueueIO(F &&f, Args &&...args)
    // {
    //     return ioPool->enqueue(std::forward<F>(f), std::forward<Args>(args)...);
    // }

private:
    ThreadPoolMgr();
    std::unique_ptr<ThreadPool> _logicPool;
};