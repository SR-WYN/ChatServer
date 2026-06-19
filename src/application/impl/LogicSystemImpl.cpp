// LogicSystemImpl.cpp
#include "LogicSystemImpl.h"
#include "CSession.h"
#include "MsgNode.h"
#include "ThreadPoolMgr.h"
#include "const.h"
#include <iostream>
#include <memory>
#include <string>

LogicSystemImpl::LogicSystemImpl(ThreadPoolMgr& thread_pool)
    : _thread_pool(thread_pool)
{
}

LogicSystemImpl::~LogicSystemImpl() = default;

void LogicSystemImpl::registerHandler(short msg_id, TcpMsgHandler handler)
{
    _handlers[msg_id] = std::move(handler);
}

void LogicSystemImpl::postMsgToQue(std::shared_ptr<LogicNode> msg)
{
    _thread_pool.enqueueLogic([this, msg]() {
        auto call_back_iter = _handlers.find(msg->getRecvNode()->getMsgId());
        if (call_back_iter == _handlers.end())
        {
            return;
        }

        call_back_iter->second(
            msg->getSession(),
            std::string(msg->getRecvNode()->getData(), msg->getRecvNode()->getCurLen()));
    });
}
