#pragma once

#include <memory>

class CSession;

// 用户会话管理器接口：uid 到 TCP 会话的映射管理
class IUserSessionManager
{
public:
    virtual ~IUserSessionManager() = default;

    virtual std::shared_ptr<CSession> getSession(int uid) = 0;
    virtual void setUserSession(int uid, std::shared_ptr<CSession> session) = 0;
    virtual void RemoveUserSession(int uid) = 0;
};
