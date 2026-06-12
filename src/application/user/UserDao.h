#pragma once

#include "Singleton.h"
#include "UserRepository.h"
#include <memory>
#include <string>

class UserDao : public UserRepository
{
public:
    std::shared_ptr<UserInfo> getUserInfo(int uid) override;
    std::shared_ptr<UserInfo> getUserInfo(const std::string &name) override;
};
