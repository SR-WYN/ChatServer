#include "HeartBeatHandler.h"
#include "CServer.h"
#include "CSession.h"
#include "const.h"
#include <chrono>
#include <iostream>
#include <json/value.h>
#include <json/writer.h>
#include <vector>

CServer *HeartBeatHandler::_server = nullptr;
std::unique_ptr<boost::asio::steady_timer> HeartBeatHandler::_idle_sweep_timer;

void HeartBeatHandler::start(CServer &server)
{
    _server = &server;
    _idle_sweep_timer = std::make_unique<boost::asio::steady_timer>(server.ioContext());
    scheduleIdleSweep();
}

void HeartBeatHandler::stop()
{
    if (_idle_sweep_timer)
    {
        boost::system::error_code ec;
        _idle_sweep_timer->cancel(ec);
        _idle_sweep_timer.reset();
    }
    _server = nullptr;
}

void HeartBeatHandler::handlePing(std::shared_ptr<CSession> session, const short &msg_id,
                                  const std::string &msg_data)
{
    (void)msg_id;
    (void)msg_data;
    touchSessionActivity(session);

    Json::Value rsp;
    rsp["error"] = ErrorCodes::SUCCESS;
    Json::FastWriter writer;
    session->send(writer.write(rsp), MSG_HEARTBEAT_PONG);
}

void HeartBeatHandler::touchSessionActivity(const std::shared_ptr<CSession> &session)
{
    if (session)
    {
        session->touchActivity();
    }
}

void HeartBeatHandler::scheduleIdleSweep()
{
    if (!_server || !_idle_sweep_timer)
    {
        return;
    }

    _idle_sweep_timer->expires_after(std::chrono::seconds(IDLE_SWEEP_INTERVAL_SECONDS));
    _idle_sweep_timer->async_wait([](const boost::system::error_code &ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            return;
        }
        if (ec)
        {
            std::cout << "heartbeat idle sweep timer: " << ec.message() << std::endl;
        }
        else
        {
            runIdleSweep();
        }
        scheduleIdleSweep();
    });
}

void HeartBeatHandler::runIdleSweep()
{
    if (!_server)
    {
        return;
    }

    std::vector<std::shared_ptr<CSession>> snapshot;
    _server->snapshotSessions(snapshot);

    const auto limit = std::chrono::seconds(SESSION_APP_IDLE_SECONDS);
    for (const auto &s : snapshot)
    {
        if (!s)
        {
            continue;
        }
        if (s->appIdleAge() >= limit)
        {
            std::cout << "closing idle session " << s->getSessionId() << std::endl;
            s->close();
        }
    }
}
