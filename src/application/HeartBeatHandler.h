#pragma once

#include <memory>
#include <string>

class CSession;

// TCP 心跳：收到 PING 回 PONG，扩展逻辑见 TODO 桩函数
class HeartBeatHandler {
public:
    static void handlePing(std::shared_ptr<CSession> session, const short &msg_id,
                           const std::string &msg_data);
    static void touchSessionActivity(const std::shared_ptr<CSession> &session);
    static void scheduleIdleSweep();

private:
    HeartBeatHandler() = delete;
};
