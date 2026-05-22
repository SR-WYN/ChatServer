#pragma once

#include "LoginRedisDao.h"
#include "CacheRedisDao.h"
#include "Singleton.h"
#include <string>

class RedisMgr : public Singleton<RedisMgr>
{
    friend class Singleton<RedisMgr>;

public:
    ~RedisMgr();
    bool get(const std::string &key, std::string &value);
    bool set(const std::string &key, const std::string &value);
    bool hSet(const char *key, const char *hkey, const char *hvalue, size_t hvaluelen);
    bool hSet(const std::string &key, const std::string &hkey, const std::string &value);
    bool hDel(const std::string &key, const std::string &field);
    void close();
    std::string hGet(const std::string &key, const std::string &hkey);

private:
    RedisMgr();
    RedisMgr(const RedisMgr &src) = delete;
    RedisMgr &operator=(const RedisMgr &src) = delete;

    CacheRedisDao _cache_dao;
    LoginRedisDao _login_dao;
};
