// NodeHeartbeat.cpp
#include "NodeHeartbeat.h"
#include "Log.h"
#include "LogModule.h"
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
    int fail_count = 0;
    while (g_running.load())
    {
        if (_client && RuntimeContext::getInstance().isInitialized())
        {
            const auto& self = RuntimeContext::getInstance().getNodeInfo();
            bool ok = _client->heartbeatNode(self.name, self.instance_uid);
            if (!ok)
            {
                ++fail_count;
                LOGW(LogModule::Net, "heartbeat failed name={} instance={} fail_count={}",
                     self.name, self.instance_uid, fail_count);
            }
            else
            {
                if (fail_count > 0)
                {
                    LOGI(LogModule::Net, "heartbeat recovered name={} instance={}", self.name,
                         self.instance_uid);
                }
                fail_count = 0;
            }
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
    LOGI(LogModule::Net, "NodeHeartbeat started");
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
    LOGI(LogModule::Net, "NodeHeartbeat stopped");
}
