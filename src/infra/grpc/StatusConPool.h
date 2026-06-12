#pragma once
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>

using message::StatusService;

using grpc::Channel;

class StatusConPool
{
public:
    StatusConPool(std::size_t pool_size, std::string host, std::string port);
    ~StatusConPool();
    std::unique_ptr<StatusService::Stub> getConnection();
    void returnConnection(std::unique_ptr<StatusService::Stub> context);
    void close();

private:
    std::atomic<bool> _stop;
    std::size_t _pool_size;
    std::string _host;
    std::string _port;
    std::queue<std::unique_ptr<StatusService::Stub>> _connections;
    std::mutex _mutex;
    std::condition_variable _cond;
};