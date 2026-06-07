#pragma once

#include "data.h"
#include <memory>
#include <string>

// 用户仓储接口：用户基础资料的持久化存储
class IUserRepository
{
public:
    virtual ~IUserRepository() = default;

    virtual std::shared_ptr<UserInfo> getUserInfo(int uid) = 0;
    virtual std::shared_ptr<UserInfo> getUserInfo(const std::string &name) = 0;
};
