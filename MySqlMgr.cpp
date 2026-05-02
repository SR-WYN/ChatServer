#include "MySqlMgr.h"

MySqlMgr::MySqlMgr()
{
}

MySqlMgr::~MySqlMgr()
{
}

std::shared_ptr<UserInfo> MySqlMgr::getUserInfo(int uid)
{
    return _dao.getUserInfo(uid);
}

std::shared_ptr<UserInfo> MySqlMgr::getUserInfo(std::string name)
{
    return _dao.getUserInfo(name);
}

bool MySqlMgr::addFriendApply(const int& uid,const int& touid)
{
    return _dao.addFriendApply(uid,touid);
}