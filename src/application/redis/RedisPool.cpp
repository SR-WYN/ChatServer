#include "RedisPool.h"
#include "ConfigMgr.h"
#include <cstddef>
#include <hiredis/hiredis.h>
#include <hiredis/read.h>
#include <mutex>

RedisPool::RedisPool() = default;

void RedisPool::initOnce()
{
    auto &pool = getInstance();
    std::lock_guard<std::mutex> lock(pool._init_mutex);
    if (pool._initialized.load())
    {
        return;
    }
    auto &cfg = ConfigMgr::getInstance();
    const auto host = cfg["Redis"]["Host"];
    const auto port = cfg["Redis"]["Port"];
    const auto pwd = cfg["Redis"]["Passwd"];
    pool.initPool(5, host.c_str(), atoi(port.c_str()), pwd.c_str());
    pool._initialized.store(true);
}

void RedisPool::initPool(std::size_t poolSize, const char *host, int port, const char *pwd)
{
    _pool_size = poolSize;
    _host = host;
    _port = port;
    _pwd = pwd;
    _b_stop.store(false);
    _counter = 0;
    _fail_count.store(0);

    for (std::size_t i = 0; i < _pool_size; i++)
    {
        auto *context = redisConnect(host, port);
        if (context == nullptr)
        {
            continue;
        }
        if (context->err != 0)
        {
            redisFree(context);
            continue;
        }

        auto reply = (redisReply *)redisCommand(context, "AUTH %s", pwd);
        if (reply == nullptr || reply->type == REDIS_REPLY_ERROR)
        {
            if (reply)
            {
                freeReplyObject(reply);
            }
            continue;
        }

        freeReplyObject(reply);
        _connections.push(context);
    }

    _check_thread = std::thread([this]() {
        while (!_b_stop)
        {
            _counter++;
            if (_counter >= 60)
            {
                checkThreadPro();
                _counter = 0;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
}

RedisPool::~RedisPool()
{
    close();
    clearConnections();
}

void RedisPool::clearConnections()
{
    std::lock_guard<std::mutex> lock(_mutex);
    while (!_connections.empty())
    {
        auto *context = _connections.front();
        redisFree(context);
        _connections.pop();
    }
}

redisContext *RedisPool::getConnection()
{
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.wait(lock, [this]() {
        if (_b_stop)
        {
            return true;
        }
        return !_connections.empty();
    });

    if (_b_stop)
    {
        return nullptr;
    }
    auto *context = _connections.front();
    _connections.pop();
    return context;
}

redisContext *RedisPool::getConNonBlock()
{
    std::unique_lock<std::mutex> lock(_mutex);
    if (_b_stop)
    {
        return nullptr;
    }
    if (_connections.empty())
    {
        return nullptr;
    }
    auto *context = _connections.front();
    _connections.pop();
    return context;
}

void RedisPool::returnConnection(redisContext *context)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_b_stop)
    {
        return;
    }
    _connections.push(context);
    _cond.notify_one();
}

void RedisPool::close()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _b_stop = true;
    _cond.notify_all();
    if (_check_thread.joinable())
    {
        _check_thread.join();
    }
}

bool RedisPool::reconnect()
{
    auto context = redisConnect(_host.c_str(), _port);
    if (context == nullptr || context->err != 0)
    {
        if (context != nullptr)
        {
            redisFree(context);
        }
        return false;
    }
    auto reply = (redisReply *)redisCommand(context, "AUTH %s", _pwd.c_str());
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR)
    {
        if (reply)
        {
            freeReplyObject(reply);
        }
        redisFree(context);
        return false;
    }
    freeReplyObject(reply);
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _connections.push(context);
    }
    return true;
}

void RedisPool::checkThreadPro()
{
    size_t poolSize;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        poolSize = _connections.size();
    }
    for (int i = 0; i < static_cast<int>(poolSize) && !_b_stop; i++)
    {
        auto *context = getConNonBlock();
        if (context == nullptr)
        {
            break;
        }
        auto reply = (redisReply *)redisCommand(context, "PING");
        if (context->err)
        {
            if (reply)
            {
                freeReplyObject(reply);
            }
            redisFree(context);
            _fail_count++;
            continue;
        }
        if (!reply || reply->type == REDIS_REPLY_ERROR)
        {
            if (reply)
            {
                freeReplyObject(reply);
            }
            redisFree(context);
            _fail_count++;
            continue;
        }
        freeReplyObject(reply);
        returnConnection(context);
    }
    while (_fail_count > 0)
    {
        if (reconnect())
        {
            _fail_count--;
        }
        else
        {
            break;
        }
    }
}
