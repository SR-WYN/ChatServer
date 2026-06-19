// LogicSystemImpl.h - TCP 消息路由实现
#pragma once

#include "LogicSystem.h"
#include <unordered_map>

class ThreadPoolMgr;

class LogicSystemImpl final : public LogicSystem
{
public:
    explicit LogicSystemImpl(ThreadPoolMgr& thread_pool);
    ~LogicSystemImpl() override;

    void registerHandler(short msg_id, TcpMsgHandler handler) override;
    void postMsgToQue(std::shared_ptr<LogicNode> msg) override;

private:
    ThreadPoolMgr& _thread_pool;
    std::unordered_map<short, TcpMsgHandler> _handlers;
};
