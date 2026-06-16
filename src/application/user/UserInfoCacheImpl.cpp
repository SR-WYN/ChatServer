#include "UserInfoCacheImpl.h"
#include "MySqlMgr.h"
#include "const.h"

bool UserInfoCacheImpl::loadFromCache(int uid, UserInfo& out)
{
    auto opt = _uid_cache.get(uid);
    if (!opt)
    {
        return false;
    }
    out = *opt;
    return true;
}

bool UserInfoCacheImpl::loadFromCache(const std::string& name, UserInfo& out)
{
    int uid = 0;
    {
        std::lock_guard<std::mutex> lock(_name_mutex);
        auto it = _name_index.find(name);
        if (it == _name_index.end())
        {
            return false;
        }
        uid = it->second;
    }
    return loadFromCache(uid, out);
}

void UserInfoCacheImpl::writeCache(const UserInfo& info)
{
    _uid_cache.put(info.uid, info);

    if (!info.name.empty())
    {
        std::lock_guard<std::mutex> lock(_name_mutex);
        _name_index[info.name] = info.uid;
    }
}

bool UserInfoCacheImpl::getByUid(int uid, std::shared_ptr<UserInfo> user_info)
{
    if (!user_info)
    {
        return false;
    }

    if (loadFromCache(uid, *user_info))
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

bool UserInfoCacheImpl::getByName(const std::string& name, std::shared_ptr<UserInfo> user_info)
{
    if (!user_info || name.empty())
    {
        return false;
    }

    if (loadFromCache(name, *user_info))
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

void UserInfoCacheImpl::fillSearchResultByUid(const std::string& uid_str, Json::Value& result)
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

void UserInfoCacheImpl::fillSearchResultByName(const std::string& name_str, Json::Value& result)
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

void UserInfoCacheImpl::fillUserJson(const UserInfo& info, Json::Value& result)
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
