#pragma once

#include "data.h"
#include <json/json.h>
#include <memory>
#include <string>

// 用户缓存查询服务接口：Redis 缓存 + MySQL 回源
class IUserCacheService
{
public:
    virtual ~IUserCacheService() = default;

    virtual bool getByUid(int uid, std::shared_ptr<UserInfo> user_info) = 0;
    virtual bool getByName(const std::string &name,
                           std::shared_ptr<UserInfo> user_info) = 0;
    virtual void fillSearchResultByUid(const std::string &uid_str,
                                       Json::Value &result) = 0;
    virtual void fillSearchResultByName(const std::string &name_str,
                                        Json::Value &result) = 0;
};
