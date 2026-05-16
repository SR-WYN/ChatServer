#pragma once

#include <boost/asio/steady_timer.hpp>
#include <memory>
#include <string>

class CServer;
class CSession;

// 应用层心跳：Ping/Pong、会话活动时间、空闲连接扫描与关闭
class HeartBeatHandler {
public:
    static void start(CServer &server);
    static void stop();

    static void handlePing(std::shared_ptr<CSession> session, const short &msg_id,
                           const std::string &msg_data);
    static void touchSessionActivity(const std::shared_ptr<CSession> &session);

private:
    HeartBeatHandler() = delete;

    static void scheduleIdleSweep();
    static void runIdleSweep();

    static CServer *_server;
    static std::unique_ptr<boost::asio::steady_timer> _idle_sweep_timer;
};
