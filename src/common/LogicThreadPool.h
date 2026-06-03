#pragma once

#include "ThreadPool.h"

// 业务逻辑线程池：专门处理 LogicSystem 投递的消息处理任务
class LogicThreadPool : public ThreadPoolBase<LogicThreadPool>
{
public:
    // CPU 密集型任务
    LogicThreadPool() : ThreadPoolBase<LogicThreadPool>(std::thread::hardware_concurrency() - 1) {}
    ~LogicThreadPool() = default;

    LogicThreadPool(const LogicThreadPool &) = delete;
    LogicThreadPool &operator=(const LogicThreadPool &) = delete;
};
