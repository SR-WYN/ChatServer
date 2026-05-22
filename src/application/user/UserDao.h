#pragma once

#include "data.h"
#include <memory>
#include <string>

class UserDao
{
public:
    std::shared_ptr<UserInfo> getUserInfo(int uid);
    std::shared_ptr<UserInfo> getUserInfo(const std::string &name);
};
