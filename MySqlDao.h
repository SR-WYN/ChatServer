#pragma once 
#include "data.h"
#include <memory>
#include <vector>

class MySqlPool;

class MySqlDao
{
public:
    MySqlDao();
    ~MySqlDao();
    std::shared_ptr<UserInfo> getUserInfo(int uid);
    std::shared_ptr<UserInfo> getUserInfo(std::string name);
    bool addFriendApply(const int& uid,const int& touid);
    bool getApplyList(const int& touid,std::vector<std::shared_ptr<ApplyInfo>> &list,int begin = 0,int limit = 10);
    bool authFriendApply(const int& uid,const int& touid);
    bool addFriend(const int& uid,const int& touid,std::string alias_name);
    bool getFriendList(int uid, std::vector<std::shared_ptr<UserInfo>> &list);
private:
    std::unique_ptr<MySqlPool> _pool;
};