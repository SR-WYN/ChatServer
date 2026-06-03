#pragma once

#include "LogicThreadPool.h"
#include "Singleton.h"

// 线程池管理器：统一管理所有类型的线程池，提供获取接口
class ThreadPoolMgr : public Singleton<ThreadPoolMgr>
{
    friend class Singleton<ThreadPoolMgr>;

public:
    // 获取业务逻辑线程池
    LogicThreadPool &getLogicPool()
    {
        return _logic_pool;
    }

private:
    ThreadPoolMgr() = default;
    ~ThreadPoolMgr() = default;

    ThreadPoolMgr(const ThreadPoolMgr &) = delete;
    ThreadPoolMgr &operator=(const ThreadPoolMgr &) = delete;

    LogicThreadPool _logic_pool; // 业务逻辑线程池实例
};
