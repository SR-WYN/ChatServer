#pragma once

#include <memory>
#include <string>
#include <vector>

class CSession;
struct ApplyInfo;
struct UserInfo;

class LoginHandler {
public:
    static void handleLogin(std::shared_ptr<CSession> session, const short &msg_id,
                            const std::string &msg_data);

private:
    LoginHandler() = delete;

    static bool getFriendApplyInfo(int touid, std::vector<std::shared_ptr<ApplyInfo>> &list);
    static bool getFriendList(int uid, std::vector<std::shared_ptr<UserInfo>> &list);
};
