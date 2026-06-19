// NodeHeartbeat.h - 节点心跳
#pragma once

#include <memory>

class StatusGrpcClient;

class NodeHeartbeat
{
public:
    static void start(std::shared_ptr<StatusGrpcClient> client);
    static void stop();

private:
    static void runLoop();

    static std::shared_ptr<StatusGrpcClient> _client;
};
