#include "UserMgr.h"
#include "CSession.h"
#include <memory>
#include <mutex>

UserMgr::UserMgr()
{

}

UserMgr::~UserMgr()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _uid_to_session.clear();
}

std::shared_ptr<CSession> UserMgr::getSession(int uid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto iter = _uid_to_session.find(uid);
    if (iter == _uid_to_session.end())
    {
        return nullptr;
    }
    return iter->second;
}

void UserMgr::setUserSession(int uid,std::shared_ptr<CSession> session)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _uid_to_session[uid] = session;
}

void UserMgr::RemoveUserSession(int uid)
{
    // auto uid_str = std::to_string(uid);
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _uid_to_session.erase(uid);
    }
}