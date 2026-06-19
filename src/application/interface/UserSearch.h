// UserSearch.h - 用户搜索业务接口
#pragma once

#include <memory>
#include <string>

class CSession;

class UserSearch
{
public:
    virtual ~UserSearch() = default;

    virtual void handle(std::shared_ptr<CSession> session, const std::string& msg_data) = 0;
};
