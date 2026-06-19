// Login.h - 登录业务接口
#pragma once

#include <memory>
#include <string>

class CSession;

class Login
{
public:
    virtual ~Login() = default;

    virtual void handle(std::shared_ptr<CSession> session, const std::string& msg_data) = 0;
};
