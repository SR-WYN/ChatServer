// UserSearchImpl.h - 用户搜索业务实现
#pragma once

#include "UserSearch.h"
#include <memory>

class UserInfoCache;

class UserSearchImpl final : public UserSearch
{
public:
    explicit UserSearchImpl(std::shared_ptr<UserInfoCache> user_info_cache);

    void handle(std::shared_ptr<CSession> session, const std::string& msg_data) override;

private:
    std::shared_ptr<UserInfoCache> _user_info_cache;
};
