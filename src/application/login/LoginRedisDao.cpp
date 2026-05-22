#include "LoginRedisDao.h"
#include "RedisPool.h"
#include "utils.h"
#include <cstring>
#include <hiredis/hiredis.h>
#include <iostream>

bool LoginRedisDao::hSet(const char *key, const char *hkey, const char *hvalue, size_t hvaluelen)
{
    auto &pool = RedisPool::getInstance();
    auto connect = pool.getConnection();
    if (connect == nullptr)
    {
        return false;
    }
    const char *argv[4];
    size_t argvlen[4];
    argv[0] = "HSET";
    argvlen[0] = 4;
    argv[1] = key;
    argvlen[1] = strlen(key);
    argv[2] = hkey;
    argvlen[2] = strlen(hkey);
    argv[3] = hvalue;
    argvlen[3] = hvaluelen;
    auto reply = (redisReply *)redisCommandArgv(connect, 4, argv, argvlen);
    if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER)
    {
        std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue
                  << " ] failure ! " << std::endl;
        if (reply)
        {
            freeReplyObject(reply);
        }
        pool.returnConnection(connect);
        return false;
    }
    std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue
              << " ] success ! " << std::endl;
    freeReplyObject(reply);
    pool.returnConnection(connect);
    return true;
}

bool LoginRedisDao::hSet(const std::string &key, const std::string &hkey, const std::string &value)
{
    return hSet(key.c_str(), hkey.c_str(), value.c_str(), value.length());
}

std::string LoginRedisDao::hGet(const std::string &key, const std::string &hkey)
{
    auto &pool = RedisPool::getInstance();
    auto connect = pool.getConnection();
    if (connect == nullptr)
    {
        return "";
    }
    const char *argv[3];
    size_t argvlen[3];
    argv[0] = "HGET";
    argvlen[0] = 4;
    argv[1] = key.c_str();
    argvlen[1] = key.length();
    argv[2] = hkey.c_str();
    argvlen[2] = hkey.length();
    auto reply = (redisReply *)redisCommandArgv(connect, 3, argv, argvlen);
    if (reply == nullptr || reply->type == REDIS_REPLY_NIL)
    {
        if (reply)
        {
            freeReplyObject(reply);
        }
        std::cout << "Execut command [ HGet " << key << " " << hkey << "  ] failure ! "
                  << std::endl;
        pool.returnConnection(connect);
        return "";
    }
    if (reply->type == REDIS_REPLY_ERROR)
    {
        std::cout << "Execut command [ HGet " << key << " " << hkey << " ] error: " << reply->str
                  << std::endl;
        freeReplyObject(reply);
        pool.returnConnection(connect);
        return "";
    }
    std::string value = reply->str;
    freeReplyObject(reply);
    std::cout << "Execut command [ HGet " << key << " " << hkey << " ] success ! " << std::endl;
    pool.returnConnection(connect);
    return value;
}

bool LoginRedisDao::hDel(const std::string &key, const std::string &field)
{
    auto &pool = RedisPool::getInstance();
    auto connect = pool.getConnection();
    if (connect == nullptr)
    {
        return false;
    }
    utils::Defer defer([&connect, &pool]() { pool.returnConnection(connect); });

    redisReply *reply =
        (redisReply *)redisCommand(connect, "HDEL %s %s", key.c_str(), field.c_str());
    if (reply == nullptr)
    {
        std::cerr << "HDEL " << key << " " << field << " failed" << std::endl;
        return false;
    }

    bool success = false;
    if (reply->type == REDIS_REPLY_INTEGER)
    {
        success = reply->integer > 0;
    }
    freeReplyObject(reply);
    return success;
}
