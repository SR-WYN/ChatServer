#pragma once

#include <string>

/** 缓存模块：登录 token、用户所在服 IP、用户资料 JSON（String） */
class CacheRedisDao
{
public:
    bool get(const std::string &key, std::string &value);
    bool set(const std::string &key, const std::string &value);
};
