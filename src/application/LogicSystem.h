#pragma once

#include "Singleton.h"
#include "data.h"
#include <functional>
#include <unordered_map>
#include <memory>

class CSession;
class LogicNode;

typedef std::function<void(std::shared_ptr<CSession>, const short &msg_id,
                           const std::string &msg_data)>
    FunCallBack;

// 业务逻辑系统：消息路由与分发，通过 ThreadPoolMgr 获取线程池处理消息
class LogicSystem : public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>;

public:
    ~LogicSystem();
    void postMsgToQue(std::shared_ptr<LogicNode> msg);

private:
    LogicSystem();
    void registerCallBacks();
    std::unordered_map<short, FunCallBack> _fun_callbacks;
};
