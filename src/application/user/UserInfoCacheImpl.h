// UserInfoCacheImpl.h - 用户信息缓存实现
// 本地 LRU 缓存 + MySQL 回源
#pragma once

#include "LRUCache.h"
#include "UserInfoCache.h"
#include "data.h"
#include <mutex>
#include <unordered_map>

/// 用户信息缓存实现 —— 本地 LRU 缓存 + MySQL 回源
class UserInfoCacheImpl : public UserInfoCache
{
public:
    UserInfoCacheImpl() = default;
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

    // uid -> UserInfo 主缓存（LRU）
    LRUCache<int, UserInfo> _uid_cache{CACHE_CAPACITY};

    // name -> uid 索引（仅做快速映射，不存储数据）
    std::mutex _name_mutex;
    std::unordered_map<std::string, int> _name_index;

    static constexpr size_t CACHE_CAPACITY = 10000; // 最大缓存 1 万用户
};
