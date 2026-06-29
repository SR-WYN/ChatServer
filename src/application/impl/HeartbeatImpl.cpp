// HeartbeatImpl.cpp
#include "HeartbeatImpl.h"
#include "CServer.h"
#include "CSession.h"
#include "Log.h"
#include "ThreadPoolMgr.h"
#include "business_constants.h"
#include "const.h"
#include <chrono>
#include <json/value.h>
#include <json/writer.h>
#include <vector>

HeartbeatImpl::HeartbeatImpl(std::shared_ptr<StatusGrpcClient> status_client,
                             std::shared_ptr<UserSessionManager> session_manager,
                             const RuntimeContext& runtime_context)
    : _status_client(std::move(status_client)),
      _session_manager(std::move(session_manager)),
      _runtime_context(runtime_context)
{
}

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

    int uid = session->getUserId();
    if (uid > 0 && _status_client && shouldRefreshTokenTTL(uid))
    {
        // 异步刷新 token TTL，避免阻塞心跳响应
        auto client = _status_client;
        ThreadPoolMgr::getInstance().enqueueGrpcClient([client, uid]() {
            if (!client->refreshTokenTTL(uid))
            {
                LOGW(LogModule::Net, "heartbeat refreshTokenTTL failed uid={}", uid);
            }
        });
    }

    Json::Value rsp;
    rsp["error"] = ErrorCodes::SUCCESS;
    Json::FastWriter writer;
    session->send(writer.write(rsp), MSG_HEARTBEAT_PONG);
}

bool HeartbeatImpl::shouldRefreshTokenTTL(int uid)
{
    std::lock_guard<std::mutex> lock(_ttl_refresh_mutex);
    auto now = std::chrono::steady_clock::now();
    auto it = _last_ttl_refresh.find(uid);
    if (it == _last_ttl_refresh.end())
    {
        _last_ttl_refresh[uid] = now;
        return true;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count();
    if (elapsed >= constants::business::kTokenRefreshThresholdSeconds)
    {
        it->second = now;
        return true;
    }
    return false;
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
