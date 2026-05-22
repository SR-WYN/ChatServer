#include "RedisMgr.h"
#include "RedisPool.h"

RedisMgr::RedisMgr()
{
    RedisPool::initOnce();
}

RedisMgr::~RedisMgr() = default;

bool RedisMgr::get(const std::string &key, std::string &value)
{
    return _cache_dao.get(key, value);
}

bool RedisMgr::set(const std::string &key, const std::string &value)
{
    return _cache_dao.set(key, value);
}

bool RedisMgr::hSet(const char *key, const char *hkey, const char *hvalue, size_t hvaluelen)
{
    return _login_dao.hSet(key, hkey, hvalue, hvaluelen);
}

std::string RedisMgr::hGet(const std::string &key, const std::string &hkey)
{
    return _login_dao.hGet(key, hkey);
}

bool RedisMgr::hSet(const std::string &key, const std::string &hkey, const std::string &value)
{
    return _login_dao.hSet(key, hkey, value);
}

void RedisMgr::close()
{
    RedisPool::getInstance().close();
}

bool RedisMgr::hDel(const std::string &key, const std::string &field)
{
    return _login_dao.hDel(key, field);
}
