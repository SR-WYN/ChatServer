#pragma once 
#include "Singleton.h"
#include "MySqlDao.h"
#include <vector>
#include <memory>
#include "data.h"

class MySqlMgr : public Singleton<MySqlMgr>
{
    friend class Singleton<MySqlMgr>;
public:
    ~MySqlMgr() override;
    std::shared_ptr<UserInfo> getUserInfo(int uid);
    std::shared_ptr<UserInfo> getUserInfo(std::string name);
    bool addFriendApply(const int &uid, const int &touid, const std::string &apply_alias_name);
    bool getFriendAlias(int self_id, int friend_id, std::string &out_alias);
    bool getFriendApplyAlias(int from_uid, int to_uid, std::string &out_alias);
    bool getApplyList(const int& touid,std::vector<std::shared_ptr<ApplyInfo>> &list,int begin = 0,int limit = 10);
    bool authFriendApply(const int& uid,const int& touid);
    bool addFriend(int applicant_uid, int accepter_uid, const std::string &alias_applicant_for_accepter,
                   const std::string &alias_accepter_for_applicant);
    bool getFriendList(int uid, std::vector<std::shared_ptr<UserInfo>> &list);
private:
    MySqlMgr();
    MySqlMgr(const MySqlMgr&) = delete;
    MySqlMgr& operator=(const MySqlMgr&) = delete;
    MySqlDao _dao;
};