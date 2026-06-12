#pragma once

#include "Singleton.h"
#include "UserSessionManager.h"
#include <memory>
#include <mutex>
#include <unordered_map>

class CSession;

class UserMgr : public Singleton<UserMgr>, public UserSessionManager
{
    friend class Singleton<UserMgr>;

public:
    ~UserMgr() override;
    std::shared_ptr<CSession> getSession(int uid) override;
    void setUserSession(int uid, std::shared_ptr<CSession> session) override;
    void removeUserSession(int uid) override;

private:
    UserMgr();
    std::mutex _mutex;
    std::unordered_map<int, std::shared_ptr<CSession>> _uid_to_session;
};
