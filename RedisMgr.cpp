#include "RedisMgr.h"
#include "ConfigMgr.h"
#include "RedisConPool.h"
#include <string.h>

RedisMgr::RedisMgr()
{
    auto &gCfgMgr = ConfigMgr::getInstance();
    auto host = gCfgMgr["Redis"]["Host"];
    auto port = gCfgMgr["Redis"]["Port"];
    auto pwd = gCfgMgr["Redis"]["Passwd"];
    _con_pool.reset(new RedisConPool(5, host.c_str(), atoi(port.c_str()), pwd.c_str()));
}

RedisMgr::~RedisMgr()
{
}

bool RedisMgr::get(const std::string &key, std::string &value)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr)
    {
        return false;
    }
    auto reply = (redisReply *)redisCommand(connect, "GET %s", key.c_str());
    if (reply == nullptr)
    {
        std::cout << "[ GET " << key << " ] failed," << connect->errstr << std::endl;
        _con_pool->returnConnection(connect);
        return false;
    }
    if (reply->type != REDIS_REPLY_STRING)
    {
        std::cout << "[ GET " << key << " ] failed" << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    value = reply->str;
    freeReplyObject(reply);
    std::cout << "Succeed to execute command [ GET " << key << " ]" << std::endl;
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::set(const std::string &key, const std::string &value)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr)
    {
        return false;
    }
    // 执行redis命令行
    auto reply = (redisReply *)redisCommand(connect, "SET %s %s", key.c_str(), value.c_str());
    // 如果返回NULL则说明执行失败
    if (NULL == reply)
    {
        std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! "
                  << std::endl;
        _con_pool->returnConnection(connect);
        return false;
    }
    // 如果执行失败则释放连接
    if (!(reply->type == REDIS_REPLY_STATUS &&
          (strcmp(reply->str, "OK") == 0 || strcmp(reply->str, "ok") == 0)))
    {
        std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! "
                  << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }
    // 执行成功 释放redisCommand执行后返回的redisReply所占用的内存
    freeReplyObject(reply);
    std::cout << "Execut command [ SET " << key << "  " << value << " ] success ! " << std::endl;
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::hSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr)
    {
        return false;
    }
    const char* argv[4];
    size_t argvlen[4];
    argv[0] = "HSET";
    argvlen[0] = 4;
    argv[1] = key;
    argvlen[1] = strlen(key);
    argv[2] = hkey;
    argvlen[2] = strlen(hkey);
    argv[3] = hvalue;
    argvlen[3] = hvaluelen;
    auto reply = (redisReply*)redisCommandArgv(connect, 4, argv, argvlen);
    if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER)
    {
        std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue
                  << " ] failure ! " << std::endl;
        if (reply) freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }
    std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue
              << " ] success ! " << std::endl;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

std::string RedisMgr::hGet(const std::string& key, const std::string& hkey)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr)
    {
        return "";
    }
    const char* argv[3];
    size_t argvlen[3];
    argv[0] = "HGET";
    argvlen[0] = 4;
    argv[1] = key.c_str();
    argvlen[1] = key.length();
    argv[2] = hkey.c_str();
    argvlen[2] = hkey.length();
    auto reply = (redisReply*)redisCommandArgv(connect, 3, argv, argvlen);
    if (reply == nullptr || reply->type == REDIS_REPLY_NIL)
    {
        if (reply) freeReplyObject(reply);
        std::cout << "Execut command [ HGet " << key << " " << hkey << "  ] failure ! "
                  << std::endl;
        _con_pool->returnConnection(connect);
        return "";
    }
    if (reply->type == REDIS_REPLY_ERROR)
    {
        std::cout << "Execut command [ HGet " << key << " " << hkey << " ] error: " << reply->str << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return "";
    }
    std::string value = reply->str;
    freeReplyObject(reply);
    std::cout << "Execut command [ HGet " << key << " " << hkey << " ] success ! " << std::endl;
    _con_pool->returnConnection(connect);
    return value;
}