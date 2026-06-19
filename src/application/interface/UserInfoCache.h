// UserInfoCache.h - 用户信息缓存接口
#pragma once

#include "data.h"
#include <json/json.h>
#include <memory>
#include <string>

class UserInfoCache
{
public:
    virtual ~UserInfoCache() = default;

    virtual bool getByUid(int uid, std::shared_ptr<UserInfo> user_info) = 0;
    virtual bool getByName(const std::string& name, std::shared_ptr<UserInfo> user_info) = 0;
    virtual void fillSearchResultByUid(const std::string& uid_str, Json::Value& result) = 0;
    virtual void fillSearchResultByName(const std::string& name_str, Json::Value& result) = 0;
};
