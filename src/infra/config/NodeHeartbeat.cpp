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

void NodeHeartbeat::runLoop()
{
    while (g_running.load())
    {
        if (RuntimeContext::getInstance().isInitialized())
        {
            const auto &self = RuntimeContext::getInstance().getNodeInfo();
            StatusGrpcClient::getInstance().heartbeatChatNode(self.name, self.instance_uid);
        }
        for (int i = 0; i < 100 && g_running.load(); ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void NodeHeartbeat::start()
{
    if (g_running.exchange(true))
    {
        return;
    }
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
}
