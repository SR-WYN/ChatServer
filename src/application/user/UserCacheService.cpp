#include "UserCacheService.h"
#include "MySqlMgr.h"
#include "RedisMgr.h"
#include "const.h"
#include <json/reader.h>
#include <json/writer.h>

std::string UserCacheService::uidBaseKey(int uid)
{
    return RedisPrefix::USER_BASE_INFO + std::to_string(uid);
}

std::string UserCacheService::nameCacheKey(const std::string &name)
{
    return RedisPrefix::USER_NAME_INFO + name;
}

bool UserCacheService::loadFromRedis(const std::string &cache_key, UserInfo &out)
{
    std::string raw;
    if (!RedisMgr::getInstance().get(cache_key, raw) || raw.empty())
    {
        return false;
    }

    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(raw, root) || !root.isObject())
    {
        return false;
    }

    out.uid = root.isMember("uid") ? root["uid"].asInt() : 0;
    out.name = root.isMember("name") ? root["name"].asString() : "";
    out.pwd = root.isMember("pwd") ? root["pwd"].asString() : "";
    out.email = root.isMember("email") ? root["email"].asString() : "";
    out.nick = root.isMember("nick") ? root["nick"].asString() : "";
    out.desc = root.isMember("desc") ? root["desc"].asString() : "";
    out.sex = root.isMember("sex") ? root["sex"].asInt() : 0;
    out.icon = root.isMember("icon") ? root["icon"].asString() : "";
    out.back = root.isMember("back") ? root["back"].asString() : "";
    out.alias_name.clear();
    return true;
}

void UserCacheService::writeCache(const UserInfo &info)
{
    Json::Value cache_root;
    cache_root["uid"] = info.uid;
    cache_root["name"] = info.name;
    cache_root["pwd"] = info.pwd;
    cache_root["email"] = info.email;
    cache_root["nick"] = info.nick;
    cache_root["desc"] = info.desc;
    cache_root["sex"] = info.sex;
    cache_root["icon"] = info.icon;

    Json::FastWriter writer;
    const std::string cache_json = writer.write(cache_root);
    (void)RedisMgr::getInstance().set(uidBaseKey(info.uid), cache_json);
    if (!info.name.empty())
    {
        (void)RedisMgr::getInstance().set(nameCacheKey(info.name), cache_json);
    }
}

void UserCacheService::fillUserJson(const UserInfo &info, Json::Value &result)
{
    result["uid"] = info.uid;
    result["name"] = info.name;
    result["pwd"] = info.pwd;
    result["email"] = info.email;
    result["nick"] = info.nick;
    result["desc"] = info.desc;
    result["sex"] = info.sex;
    result["icon"] = info.icon;
}

bool UserCacheService::getByUid(int uid, std::shared_ptr<UserInfo> user_info)
{
    if (!user_info)
    {
        return false;
    }

    if (loadFromRedis(uidBaseKey(uid), *user_info))
    {
        return true;
    }

    auto db_user = MySqlMgr::getInstance().users().getUserInfo(uid);
    if (!db_user)
    {
        return false;
    }

    *user_info = *db_user;
    if (user_info->nick.empty())
    {
        user_info->nick = user_info->name;
    }
    user_info->alias_name.clear();
    writeCache(*user_info);
    return true;
}

bool UserCacheService::getByName(const std::string &name, std::shared_ptr<UserInfo> user_info)
{
    if (!user_info || name.empty())
    {
        return false;
    }

    if (loadFromRedis(nameCacheKey(name), *user_info))
    {
        return true;
    }

    auto db_user = MySqlMgr::getInstance().users().getUserInfo(name);
    if (!db_user)
    {
        return false;
    }

    *user_info = *db_user;
    if (user_info->nick.empty())
    {
        user_info->nick = user_info->name;
    }
    user_info->alias_name.clear();
    writeCache(*user_info);
    return true;
}

void UserCacheService::fillSearchResultByUid(const std::string &uid_str, Json::Value &result)
{
    result["error"] = ErrorCodes::SUCCESS;
    int uid = 0;
    try
    {
        uid = std::stoi(uid_str);
    }
    catch (...)
    {
        result["error"] = ErrorCodes::UID_INVALID;
        return;
    }

    auto user_info = std::make_shared<UserInfo>();
    if (!getByUid(uid, user_info))
    {
        result["error"] = ErrorCodes::UID_INVALID;
        return;
    }

    fillUserJson(*user_info, result);
}

void UserCacheService::fillSearchResultByName(const std::string &name_str, Json::Value &result)
{
    result["error"] = ErrorCodes::SUCCESS;

    auto user_info = std::make_shared<UserInfo>();
    if (!getByName(name_str, user_info))
    {
        result["error"] = ErrorCodes::UID_INVALID;
        return;
    }

    fillUserJson(*user_info, result);
}
