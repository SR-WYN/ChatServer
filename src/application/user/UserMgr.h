#pragma once

#include "IUserSessionManager.h"
#include "Singleton.h"
#include <memory>
#include <mutex>
#include <unordered_map>

class CSession;

class UserMgr : public Singleton<UserMgr>, public IUserSessionManager
{
    friend class Singleton<UserMgr>;
public:
    ~UserMgr() override;
    std::shared_ptr<CSession> getSession(int uid) override;
    void setUserSession(int uid,std::shared_ptr<CSession> session) override;
    void RemoveUserSession(int uid) override;
private:
    UserMgr();
    std::mutex _mutex;
    std::unordered_map<int, std::shared_ptr<CSession>> _uid_to_session;
};
