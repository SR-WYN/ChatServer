// UserRepository.h - 用户基础资料持久化存储接口
#pragma once

#include "data.h"
#include <memory>
#include <string>

/// 用户仓储接口
class UserRepository
{
public:
    virtual ~UserRepository() = default;

    virtual std::shared_ptr<UserInfo> getUserInfo(int uid) = 0;
    virtual std::shared_ptr<UserInfo> getUserInfo(const std::string &name) = 0;
};
