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
private:
    std::unique_ptr<MySqlPool> _pool;
};