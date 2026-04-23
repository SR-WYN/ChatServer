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
private:
    RedisMgr();
    RedisMgr(const RedisMgr& src) = delete;
    RedisMgr& operator=(const RedisMgr& src) = delete;
    std::unique_ptr<RedisConPool> _con_pool;
};