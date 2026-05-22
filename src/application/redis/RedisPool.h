#pragma once

#include "Singleton.h"
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <hiredis/hiredis.h>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

class RedisPool : public Singleton<RedisPool>
{
    friend class Singleton<RedisPool>;

public:
    static void initOnce();
    ~RedisPool() override;
    redisContext *getConnection();
    redisContext *getConNonBlock();
    void returnConnection(redisContext *context);
    void close();

private:
    RedisPool();
    RedisPool(const RedisPool &) = delete;
    RedisPool &operator=(const RedisPool &) = delete;
    void initPool(std::size_t poolSize, const char *host, int port, const char *pwd);
    void clearConnections();
    bool reconnect();
    void checkThreadPro();

    std::mutex _init_mutex;
    std::atomic<bool> _initialized{false};
    std::atomic<bool> _b_stop{false};
    std::size_t _pool_size{0};
    std::string _host;
    std::string _pwd;
    int _port{0};
    std::queue<redisContext *> _connections;
    std::mutex _mutex;
    std::condition_variable _cond;
    std::thread _check_thread;
    std::atomic<int> _fail_count{0};
    int _counter{0};
};
