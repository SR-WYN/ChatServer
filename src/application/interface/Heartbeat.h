// Heartbeat.h - 心跳业务接口
#pragma once

#include <memory>
#include <string>

class CServer;
class CSession;

class Heartbeat
{
public:
    virtual ~Heartbeat() = default;

    virtual void start(CServer& server) = 0;
    virtual void stop() = 0;
    virtual void handlePing(std::shared_ptr<CSession> session, const std::string& msg_data) = 0;
    virtual void touchSessionActivity(const std::shared_ptr<CSession>& session) = 0;
};
