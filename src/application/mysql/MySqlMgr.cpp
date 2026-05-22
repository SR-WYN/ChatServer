#include "MySqlMgr.h"
#include "MySqlPool.h"

MySqlMgr::MySqlMgr()
{
    MySqlPool::initOnce();
}

MySqlMgr::~MySqlMgr() = default;

std::shared_ptr<UserInfo> MySqlMgr::getUserInfo(int uid)
{
    return _user_dao.getUserInfo(uid);
}

std::shared_ptr<UserInfo> MySqlMgr::getUserInfo(const std::string &name)
{
    return _user_dao.getUserInfo(name);
}

bool MySqlMgr::addFriendApply(const int &uid, const int &touid, const std::string &apply_alias_name)
{
    return _friend_dao.addFriendApply(uid, touid, apply_alias_name);
}

bool MySqlMgr::getFriendAlias(int self_id, int friend_id, std::string &out_alias)
{
    return _friend_dao.getFriendAlias(self_id, friend_id, out_alias);
}

bool MySqlMgr::getFriendApplyAlias(int from_uid, int to_uid, std::string &out_alias)
{
    return _friend_dao.getFriendApplyAlias(from_uid, to_uid, out_alias);
}

bool MySqlMgr::getApplyList(const int &touid, std::vector<std::shared_ptr<ApplyInfo>> &list,
                             int begin, int limit)
{
    return _friend_dao.getApplyList(touid, list, begin, limit);
}

bool MySqlMgr::authFriendApply(const int &uid, const int &touid)
{
    return _friend_dao.authFriendApply(uid, touid);
}

bool MySqlMgr::addFriend(int applicant_uid, int accepter_uid,
                         const std::string &alias_applicant_for_accepter,
                         const std::string &alias_accepter_for_applicant)
{
    return _friend_dao.addFriend(applicant_uid, accepter_uid, alias_applicant_for_accepter,
                                 alias_accepter_for_applicant);
}

bool MySqlMgr::getFriendList(int uid, std::vector<std::shared_ptr<UserInfo>> &list)
{
    return _friend_dao.getFriendList(uid, list);
}

IChatMessageRepository &MySqlMgr::chatMessages()
{
    return _chat_message_dao;
}
