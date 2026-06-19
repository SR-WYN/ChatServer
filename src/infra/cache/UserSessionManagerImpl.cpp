// UserSessionManagerImpl.cpp
#include "UserSessionManagerImpl.h"
#include "CSession.h"

UserSessionManagerImpl::~UserSessionManagerImpl()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _uid_to_session.clear();
}

std::shared_ptr<CSession> UserSessionManagerImpl::getSession(int uid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto iter = _uid_to_session.find(uid);
    if (iter == _uid_to_session.end())
    {
        return nullptr;
    }
    return iter->second;
}

void UserSessionManagerImpl::setUserSession(int uid, std::shared_ptr<CSession> session)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _uid_to_session[uid] = std::move(session);
}

void UserSessionManagerImpl::removeUserSession(int uid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _uid_to_session.erase(uid);
}
