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
    bool addFriendApply(const int& uid,const int& touid);
    bool getApplyList(const int& touid,std::vector<std::shared_ptr<ApplyInfo>> &list,int begin = 0,int limit = 10);
private:
    MySqlMgr();
    MySqlMgr(const MySqlMgr&) = delete;
    MySqlMgr& operator=(const MySqlMgr&) = delete;
    MySqlDao _dao;
};