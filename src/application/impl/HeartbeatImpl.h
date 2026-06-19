// HeartbeatImpl.h - 心跳业务实现
#pragma once

#include "Heartbeat.h"
#include <boost/asio/steady_timer.hpp>
#include <memory>

class HeartbeatImpl final : public Heartbeat
{
public:
    HeartbeatImpl() = default;
    ~HeartbeatImpl() override;

    void start(CServer& server) override;
    void stop() override;
    void handlePing(std::shared_ptr<CSession> session, const std::string& msg_data) override;
    void touchSessionActivity(const std::shared_ptr<CSession>& session) override;

private:
    void scheduleIdleSweep();
    void runIdleSweep();

    CServer* _server = nullptr;
    std::unique_ptr<boost::asio::steady_timer> _idle_sweep_timer;
};
