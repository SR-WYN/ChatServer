// UserInfoCacheImpl.h - 用户信息缓存实现
#pragma once

#include "LRUCache.h"
#include "UserInfoCache.h"
#include "business_constants.h"
#include "data.h"
#include <memory>
#include <mutex>
#include <unordered_map>

class MySqlMgr;

class UserInfoCacheImpl final : public UserInfoCache
{
public:
    explicit UserInfoCacheImpl(std::shared_ptr<MySqlMgr> mysql_mgr);
    ~UserInfoCacheImpl() override = default;

    bool getByUid(int uid, std::shared_ptr<UserInfo> user_info) override;
    bool getByName(const std::string& name, std::shared_ptr<UserInfo> user_info) override;
    void fillSearchResultByUid(const std::string& uid_str, Json::Value& result) override;
    void fillSearchResultByName(const std::string& name_str, Json::Value& result) override;

private:
    bool loadFromCache(int uid, UserInfo& out);
    bool loadFromCache(const std::string& name, UserInfo& out);
    void writeCache(const UserInfo& info);
    void fillUserJson(const UserInfo& info, Json::Value& result);

    std::shared_ptr<MySqlMgr> _mysql_mgr;
    LRUCache<int, UserInfo> _uid_cache{constants::business::kUserInfoCacheCapacity};
    std::mutex _name_mutex;
    std::unordered_map<std::string, int> _name_index;
};
