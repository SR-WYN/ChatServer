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

bool MySqlMgr::getApplyList(const int& touid,std::vector<std::shared_ptr<ApplyInfo>> &list,int begin,int limit)
{
    return _dao.getApplyList(touid,list,begin,limit);
}