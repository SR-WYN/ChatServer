// UserSessionManager.h - 用户会话管理器接口
#pragma once

#include <memory>

class CSession;

class UserSessionManager
{
public:
    virtual ~UserSessionManager() = default;

    virtual std::shared_ptr<CSession> getSession(int uid) = 0;
    virtual void setUserSession(int uid, std::shared_ptr<CSession> session) = 0;
    virtual void removeUserSession(int uid) = 0;
};
