// UserInfoCache.h - 用户信息缓存接口
// 定义用户资料的本地缓存查询能力，与具体缓存实现解耦
#pragma once

#include "data.h"
#include <json/json.h>
#include <memory>
#include <string>

/// 用户信息缓存接口
class UserInfoCache
{
public:
    virtual ~UserInfoCache() = default;

    /// 根据 uid 查询用户信息
    /// @param uid 用户 ID
    /// @param user_info 输出参数，存储查询结果
    /// @return 查询成功返回 true，用户不存在返回 false
    virtual bool getByUid(int uid, std::shared_ptr<UserInfo> user_info) = 0;

    /// 根据用户名查询用户信息
    /// @param name 用户名
    /// @param user_info 输出参数，存储查询结果
    /// @return 查询成功返回 true，用户不存在返回 false
    virtual bool getByName(const std::string& name, std::shared_ptr<UserInfo> user_info) = 0;

    /// 根据 uid 填充搜索结果 JSON
    /// @param uid_str 用户 ID 字符串
    /// @param result 输出 JSON，包含 error 和用户信息字段
    virtual void fillSearchResultByUid(const std::string& uid_str, Json::Value& result) = 0;

    /// 根据用户名填充搜索结果 JSON
    /// @param name_str 用户名字符串
    /// @param result 输出 JSON，包含 error 和用户信息字段
    virtual void fillSearchResultByName(const std::string& name_str, Json::Value& result) = 0;
};
