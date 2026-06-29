// HeartbeatImpl.h - 心跳业务实现
#pragma once

#include "Heartbeat.h"
#include "RuntimeContext.h"
#include "StatusGrpcClient.h"
#include "UserSessionManager.h"
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <memory>

class HeartbeatImpl final : public Heartbeat
{
public:
    HeartbeatImpl(std::shared_ptr<StatusGrpcClient> status_client,
                  std::shared_ptr<UserSessionManager> session_manager,
                  const RuntimeContext& runtime_context);
    ~HeartbeatImpl() override;

    void start(CServer& server) override;
    void stop() override;
    void handlePing(std::shared_ptr<CSession> session, const std::string& msg_data) override;
    void touchSessionActivity(const std::shared_ptr<CSession>& session) override;

private:
    void scheduleIdleSweep();
    void runIdleSweep();
    bool shouldRefreshTokenTTL(int uid);

    CServer* _server = nullptr;
    std::unique_ptr<boost::asio::steady_timer> _idle_sweep_timer;
    std::shared_ptr<StatusGrpcClient> _status_client;
    std::shared_ptr<UserSessionManager> _session_manager;
    const RuntimeContext& _runtime_context;

    // 每个 uid 上次刷新 token TTL 的时间戳
    std::unordered_map<int, std::chrono::steady_clock::time_point> _last_ttl_refresh;
    std::mutex _ttl_refresh_mutex;
};
