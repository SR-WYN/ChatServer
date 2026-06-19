#pragma once
#include "Singleton.h"
#include "ThreadPool.h"
#include <memory>

class ThreadPoolMgr : public Singleton<ThreadPoolMgr>
{
    friend class Singleton<ThreadPoolMgr>;

public:
    ~ThreadPoolMgr();

    void enqueueLogic(std::function<void()> task);
    void enqueuePersist(std::function<void()> task);

private:
    ThreadPoolMgr();
    std::unique_ptr<ThreadPool> _logicPool;
    std::unique_ptr<ThreadPool> _persistPool;
};