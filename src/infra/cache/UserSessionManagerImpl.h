// UserSessionManagerImpl.h - 用户会话管理器实现
#pragma once

#include "UserSessionManager.h"
#include <memory>
#include <mutex>
#include <unordered_map>

class UserSessionManagerImpl final : public UserSessionManager
{
public:
    UserSessionManagerImpl() = default;
    ~UserSessionManagerImpl() override;

    std::shared_ptr<CSession> getSession(int uid) override;
    void setUserSession(int uid, std::shared_ptr<CSession> session) override;
    void removeUserSession(int uid) override;

private:
    std::mutex _mutex;
    std::unordered_map<int, std::shared_ptr<CSession>> _uid_to_session;
};
