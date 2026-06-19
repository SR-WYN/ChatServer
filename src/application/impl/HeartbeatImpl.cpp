// HeartbeatImpl.cpp
#include "HeartbeatImpl.h"
#include "CServer.h"
#include "CSession.h"
#include "Log.h"
#include "const.h"
#include <chrono>
#include <json/value.h>
#include <json/writer.h>
#include <vector>

HeartbeatImpl::~HeartbeatImpl()
{
    stop();
}

void HeartbeatImpl::start(CServer& server)
{
    _server = &server;
    _idle_sweep_timer = std::make_unique<boost::asio::steady_timer>(server.ioContext());
    scheduleIdleSweep();
    LOGI(LogModule::Net, "HeartbeatImpl started, idle sweep interval={}s",
         IDLE_SWEEP_INTERVAL_SECONDS);
}

void HeartbeatImpl::stop()
{
    if (_idle_sweep_timer)
    {
        boost::system::error_code ec;
        _idle_sweep_timer->cancel(ec);
        _idle_sweep_timer.reset();
    }
    _server = nullptr;
    LOGI(LogModule::Net, "HeartbeatImpl stopped");
}

void HeartbeatImpl::handlePing(std::shared_ptr<CSession> session, const std::string& msg_data)
{
    (void)msg_data;
    touchSessionActivity(session);

    Json::Value rsp;
    rsp["error"] = ErrorCodes::SUCCESS;
    Json::FastWriter writer;
    session->send(writer.write(rsp), MSG_HEARTBEAT_PONG);
}

void HeartbeatImpl::touchSessionActivity(const std::shared_ptr<CSession>& session)
{
    if (session)
    {
        session->touchActivity();
    }
}

void HeartbeatImpl::scheduleIdleSweep()
{
    if (!_server || !_idle_sweep_timer)
    {
        return;
    }

    _idle_sweep_timer->expires_after(std::chrono::seconds(IDLE_SWEEP_INTERVAL_SECONDS));
    _idle_sweep_timer->async_wait([this](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            return;
        }
        if (ec)
        {
            LOGE(LogModule::Net, "idle sweep timer error: {}", ec.message());
        }
        else
        {
            runIdleSweep();
        }
        scheduleIdleSweep();
    });
}

void HeartbeatImpl::runIdleSweep()
{
    if (!_server)
    {
        return;
    }

    std::vector<std::shared_ptr<CSession>> snapshot;
    _server->snapshotSessions(snapshot);

    const auto limit = std::chrono::seconds(SESSION_APP_IDLE_SECONDS);
    int closed = 0;
    for (const auto& s : snapshot)
    {
        if (!s)
        {
            LOGW(LogModule::Net, "idle sweep encountered null session");
            continue;
        }
        if (s->appIdleAge() >= limit)
        {
            s->close();
            ++closed;
        }
    }

    if (closed > 0)
    {
        LOGI(LogModule::Net, "idle sweep closed {} sessions (timeout={}s)", closed,
             SESSION_APP_IDLE_SECONDS);
    }
}
