#pragma once

#include <cstddef>
#include <string>

/** 各 Chat 服登录人数统计（Hash：LOGIN_COUNT） */
class LoginRedisDao
{
public:
    bool hSet(const char *key, const char *hkey, const char *hvalue, size_t hvaluelen);
    bool hSet(const std::string &key, const std::string &hkey, const std::string &value);
    std::string hGet(const std::string &key, const std::string &hkey);
    bool hDel(const std::string &key, const std::string &field);
};
