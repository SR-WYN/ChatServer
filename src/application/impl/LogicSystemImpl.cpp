// LogicSystemImpl.cpp
#include "LogicSystemImpl.h"
#include "CSession.h"
#include "Log.h"
#include "LogModule.h"
#include "MsgNode.h"
#include "ThreadPoolMgr.h"
#include "const.h"
#include <chrono>
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
    LOGI(LogModule::Logic, "registered TCP handler msg_id={}", msg_id);
}

void LogicSystemImpl::postMsgToQue(std::shared_ptr<LogicNode> msg)
{
    auto session = msg->getSession();
    const short msg_id = msg->getRecvNode()->getMsgId();
    const int body_len = msg->getRecvNode()->getCurLen();
    const std::string session_id = session ? session->getSessionId() : "unknown";
    const int uid = session ? session->getUserId() : 0;

    LOGI(LogModule::Logic,
         "dispatching msg_id={} session={} uid={} body_len={} to logic pool", msg_id, session_id,
         uid, body_len);

    _thread_pool.enqueueLogic([this, msg, msg_id, session_id, uid]() {
        const auto start = std::chrono::steady_clock::now();
        auto call_back_iter = _handlers.find(msg_id);
        if (call_back_iter == _handlers.end())
        {
            LOGW(LogModule::Logic, "no handler for msg_id={} session={} uid={}", msg_id, session_id,
                 uid);
            return;
        }

        try
        {
            call_back_iter->second(
                msg->getSession(),
                std::string(msg->getRecvNode()->getData(), msg->getRecvNode()->getCurLen()));
        }
        catch (std::exception& e)
        {
            LOGE(LogModule::Logic, "handler exception for msg_id={} session={} uid={}: {}", msg_id,
                 session_id, uid, e.what());
        }

        const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::steady_clock::now() - start)
                                 .count();
        LOGI(LogModule::Logic, "handled msg_id={} session={} uid={} cost={}ms", msg_id, session_id,
             uid, cost_ms);
    });
}
