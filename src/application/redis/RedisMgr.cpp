#include "RedisMgr.h"
#include "RedisPool.h"
#include "RedisSession.h"

RedisMgr::RedisMgr()
{
    RedisPool::initOnce();
}

RedisMgr::~RedisMgr() = default;

bool RedisMgr::get(const std::string &key, std::string &value)
{
    return RedisSession::get(key, value);
}

bool RedisMgr::set(const std::string &key, const std::string &value)
{
    return RedisSession::set(key, value);
}

bool RedisMgr::hSet(const char *key, const char *field, const char *val, size_t vallen)
{
    return RedisSession::hSet(std::string(key), std::string(field), std::string(val, vallen));
}

std::string RedisMgr::hGet(const std::string &key, const std::string &field)
{
    return RedisSession::hGet(key, field);
}

bool RedisMgr::hSet(const std::string &key, const std::string &field, const std::string &value)
{
    return RedisSession::hSet(key, field, value);
}

void RedisMgr::close()
{
    RedisPool::getInstance().close();
}

bool RedisMgr::hDel(const std::string &key, const std::string &field)
{
    return RedisSession::hDel(key, field);
}
