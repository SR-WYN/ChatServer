#pragma once
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <atomic>
#include <condition_variable>
#include <grpcpp/channel.h>
#include <grpcpp/grpcpp.h>
#include <hiredis/hiredis.h>
#include <mutex>
#include <queue>
#include <thread>

class RedisConPool
{
public:
    RedisConPool(std::size_t poolSize,const char* host,int port,const char* pwd);
    ~RedisConPool();
    void clearConnections();
    redisContext* getConnection();
    redisContext* getConNonBlock();
    void returnConnection(redisContext* context);
    void close();

private:
    bool reconnect();
    void checkThreadPro();
    std::atomic<bool> _b_stop;
    std::size_t _pool_size;
    std::string _host;
    std::string _pwd;
    int _port;
    std::queue<redisContext *> _connections;
    std::mutex _mutex;
    std::condition_variable _cond;
    std::thread _check_thread;
    std::atomic<int> _fail_count;
    int _counter;
};