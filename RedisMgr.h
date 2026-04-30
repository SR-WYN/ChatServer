#pragma once
#include "Singleton.h"
#include <hiredis/hiredis.h>
#include <memory>
#include <string>

class RedisConPool;

class RedisMgr : public Singleton<RedisMgr>
{
    friend class Singleton<RedisMgr>;
public:
    ~RedisMgr();
    bool get(const std::string& key, std::string& value);
    bool set(const std::string& key, const std::string& value);
    bool hSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
    std::string hGet(const std::string& key, const std::string& hkey);
private:
    RedisMgr();
    RedisMgr(const RedisMgr& src) = delete;
    RedisMgr& operator=(const RedisMgr& src) = delete;
    std::unique_ptr<RedisConPool> _con_pool;
};