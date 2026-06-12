#pragma once

#include <memory>
#include <string>

class CSession;

class UserHandler
{
public:
    static void handleSearchUser(std::shared_ptr<CSession> session, const short &msg_id,
                                 const std::string &msg_data);

private:
    UserHandler() = delete;
};
