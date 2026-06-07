#pragma once

#include "IUserRepository.h"
#include "data.h"
#include <memory>
#include <string>

class UserDao : public IUserRepository
{
public:
    std::shared_ptr<UserInfo> getUserInfo(int uid) override;
    std::shared_ptr<UserInfo> getUserInfo(const std::string &name) override;
};
