// Friend.h - 好友业务接口
#pragma once

#include <memory>
#include <string>

class CSession;

class Friend
{
public:
    virtual ~Friend() = default;

    virtual void handleAddFriend(std::shared_ptr<CSession> session,
                                 const std::string& msg_data) = 0;
    virtual void handleAuthFriend(std::shared_ptr<CSession> session,
                                  const std::string& msg_data) = 0;
};
