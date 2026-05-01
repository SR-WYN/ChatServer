#pragma once 
#include "Singleton.h"
#include "MySqlDao.h"

class MySqlMgr : public Singleton<MySqlMgr>
{
    friend class Singleton<MySqlMgr>;
public:
    ~MySqlMgr() override;
    std::shared_ptr<UserInfo> getUserInfo(int uid);
    std::shared_ptr<UserInfo> getUserInfo(std::string name);
private:
    MySqlMgr();
    MySqlMgr(const MySqlMgr&) = delete;
    MySqlMgr& operator=(const MySqlMgr&) = delete;
    MySqlDao _dao;
};