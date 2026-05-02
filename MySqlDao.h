#pragma once 
#include "data.h"
#include <memory>

class MySqlPool;

class MySqlDao
{
public:
    MySqlDao();
    ~MySqlDao();
    std::shared_ptr<UserInfo> getUserInfo(int uid);
    std::shared_ptr<UserInfo> getUserInfo(std::string name);
    bool addFriendApply(const int& uid,const int& touid);
private:
    std::unique_ptr<MySqlPool> _pool;
};