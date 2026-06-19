#include "ThreadPoolMgr.h"

ThreadPoolMgr::ThreadPoolMgr()
{
    _logicPool = std::make_unique<ThreadPool>(std::thread::hardware_concurrency() - 1);

    // ioPool = std::make_unique<ThreadPool>(std::thread::hardware_concurrency() * 2);
}

ThreadPoolMgr::~ThreadPoolMgr() = default;

void ThreadPoolMgr::enqueueLogic(std::function<void()> task)
{
    _logicPool->enqueue(std::move(task));
}