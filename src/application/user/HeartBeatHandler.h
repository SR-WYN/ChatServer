#pragma once

#include <boost/asio/steady_timer.hpp>
#include <memory>
#include <string>

class CServer;
class CSession;

// 应用层心跳管理：Ping/Pong 响应、会话活动时间更新、空闲连接定期扫描与关闭
class HeartBeatHandler
{
public:
    // 启动心跳管理（创建定时器，开始空闲扫描）
    static void start(CServer &server);

    // 停止心跳管理（取消定时器）
    static void stop();

    // 处理客户端 Ping，回复 Pong 并更新会话活动时间
    static void handlePing(std::shared_ptr<CSession> session, const short &msg_id,
                           const std::string &msg_data);

    // 更新会话的最后活动时间
    static void touchSessionActivity(const std::shared_ptr<CSession> &session);

private:
    HeartBeatHandler() = delete;

    // 安排下一次空闲连接扫描
    static void scheduleIdleSweep();

    // 执行空闲连接扫描，关闭超时会话
    static void runIdleSweep();

    static CServer *_server;                                             // 关联的 TCP 服务器
    static std::unique_ptr<boost::asio::steady_timer> _idle_sweep_timer; // 空闲扫描定时器
};
