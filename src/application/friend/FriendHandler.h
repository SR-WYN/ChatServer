#pragma once

#include <memory>
#include <string>

class CSession;

class FriendHandler {
public:
    static void handleAddFriend(std::shared_ptr<CSession> session, const short &msg_id,
                                const std::string &msg_data);
    static void handleAuthFriend(std::shared_ptr<CSession> session, const short &msg_id,
                                 const std::string &msg_data);

private:
    FriendHandler() = delete;
};
