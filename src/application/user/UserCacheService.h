#pragma once

#include "data.h"
#include <json/json.h>
#include <memory>
#include <string>

// 用户基础资料：Redis 缓存 + MySQL 回源，供 LogicSystem / ChatServiceImpl 等共用
class UserCacheService
{
public:
    UserCacheService() = delete;

    // Redis -> MySQL -> 回写缓存；user_info 不可为空
    static bool getByUid(int uid, std::shared_ptr<UserInfo> user_info);
    static bool getByName(const std::string &name, std::shared_ptr<UserInfo> user_info);

    // 搜索用户回包（MSG_SEARCH_USER_RSP）
    static void fillSearchResultByUid(const std::string &uid_str, Json::Value &result);
    static void fillSearchResultByName(const std::string &name_str, Json::Value &result);

private:
    static std::string uidBaseKey(int uid);
    static std::string nameCacheKey(const std::string &name);
    static bool loadFromRedis(const std::string &cache_key, UserInfo &out);
    static void writeCache(const UserInfo &info);
    static void fillUserJson(const UserInfo &info, Json::Value &result);
};
