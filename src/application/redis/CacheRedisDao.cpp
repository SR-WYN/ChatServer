#include "CacheRedisDao.h"
#include "RedisPool.h"
#include <cstring>
#include <hiredis/hiredis.h>
#include <iostream>

bool CacheRedisDao::get(const std::string &key, std::string &value)
{
    auto &pool = RedisPool::getInstance();
    auto connect = pool.getConnection();
    if (connect == nullptr)
    {
        return false;
    }
    auto reply = (redisReply *)redisCommand(connect, "GET %s", key.c_str());
    if (reply == nullptr)
    {
        std::cout << "[ GET " << key << " ] failed," << connect->errstr << std::endl;
        pool.returnConnection(connect);
        return false;
    }
    if (reply->type != REDIS_REPLY_STRING)
    {
        std::cout << "[ GET " << key << " ] failed" << std::endl;
        freeReplyObject(reply);
        pool.returnConnection(connect);
        return false;
    }

    value = reply->str;
    freeReplyObject(reply);
    std::cout << "Succeed to execute command [ GET " << key << " ]" << std::endl;
    pool.returnConnection(connect);
    return true;
}

bool CacheRedisDao::set(const std::string &key, const std::string &value)
{
    auto &pool = RedisPool::getInstance();
    auto connect = pool.getConnection();
    if (connect == nullptr)
    {
        return false;
    }
    auto reply = (redisReply *)redisCommand(connect, "SET %s %s", key.c_str(), value.c_str());
    if (reply == nullptr)
    {
        std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! "
                  << std::endl;
        pool.returnConnection(connect);
        return false;
    }
    if (!(reply->type == REDIS_REPLY_STATUS &&
          (strcmp(reply->str, "OK") == 0 || strcmp(reply->str, "ok") == 0)))
    {
        std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! "
                  << std::endl;
        freeReplyObject(reply);
        pool.returnConnection(connect);
        return false;
    }
    freeReplyObject(reply);
    std::cout << "Execut command [ SET " << key << "  " << value << " ] success ! " << std::endl;
    pool.returnConnection(connect);
    return true;
}
