// LogicSystem.h - TCP 消息路由接口
#pragma once

#include <functional>
#include <memory>
#include <string>

class CSession;
class LogicNode;

using TcpMsgHandler = std::function<void(std::shared_ptr<CSession>, const std::string& msg_data)>;

class LogicSystem
{
public:
    virtual ~LogicSystem() = default;

    virtual void registerHandler(short msg_id, TcpMsgHandler handler) = 0;
    virtual void postMsgToQue(std::shared_ptr<LogicNode> msg) = 0;
};
