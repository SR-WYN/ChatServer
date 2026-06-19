// NodeHeartbeat.cpp
#include "NodeHeartbeat.h"
#include "RuntimeContext.h"
#include "StatusGrpcClient.h"
#include <atomic>
#include <chrono>
#include <thread>

namespace
{
std::atomic<bool> g_running{false};
std::thread g_worker;
} // namespace

std::shared_ptr<StatusGrpcClient> NodeHeartbeat::_client;

void NodeHeartbeat::runLoop()
{
    while (g_running.load())
    {
        if (_client && RuntimeContext::getInstance().isInitialized())
        {
            const auto& self = RuntimeContext::getInstance().getNodeInfo();
            _client->heartbeatChatNode(self.name, self.instance_uid);
        }
        for (int i = 0; i < 100 && g_running.load(); ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void NodeHeartbeat::start(std::shared_ptr<StatusGrpcClient> client)
{
    if (g_running.exchange(true))
    {
        return;
    }
    _client = std::move(client);
    g_worker = std::thread(runLoop);
}

void NodeHeartbeat::stop()
{
    if (!g_running.exchange(false))
    {
        return;
    }
    if (g_worker.joinable())
    {
        g_worker.join();
    }
    _client.reset();
}
