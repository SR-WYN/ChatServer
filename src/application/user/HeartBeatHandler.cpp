#include "HeartBeatHandler.h"
#include "CServer.h"
#include "CSession.h"
#include "Log.h"
#include "const.h"
#include <chrono>
#include <json/value.h>
#include <json/writer.h>
#include <vector>

CServer *HeartBeatHandler::_server = nullptr;
std::unique_ptr<boost::asio::steady_timer> HeartBeatHandler::_idle_sweep_timer;

// 启动心跳管理：保存 server 引用，创建定时器并开始空闲扫描
void HeartBeatHandler::start(CServer &server)
{
    _server = &server;
    _idle_sweep_timer = std::make_unique<boost::asio::steady_timer>(server.ioContext());
    scheduleIdleSweep();
    LOGI(LogModule::Net, "HeartBeatHandler started, idle sweep interval={}s", IDLE_SWEEP_INTERVAL_SECONDS);
}

// 停止心跳管理：取消定时器，清空引用
void HeartBeatHandler::stop()
{
    if (_idle_sweep_timer)
    {
        boost::system::error_code ec;
        _idle_sweep_timer->cancel(ec);
        _idle_sweep_timer.reset();
    }
    _server = nullptr;
    LOGI(LogModule::Net, "HeartBeatHandler stopped");
}

// 处理客户端 Ping：更新会话活动时间，回复 Pong
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

// 更新会话的最后活动时间
void HeartBeatHandler::touchSessionActivity(const std::shared_ptr<CSession> &session)
{
    if (session)
    {
        session->touchActivity();
    }
}

// 安排下一次空闲连接扫描
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
            // 定时器被取消（stop 时触发），直接返回
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

// 执行空闲连接扫描：遍历所有会话，关闭超过空闲阈值的连接
void HeartBeatHandler::runIdleSweep()
{
    if (!_server)
    {
        return;
    }

    std::vector<std::shared_ptr<CSession>> snapshot;
    _server->snapshotSessions(snapshot);

    const auto limit = std::chrono::seconds(SESSION_APP_IDLE_SECONDS);
    int closed = 0;
    for (const auto &s : snapshot)
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
        LOGI(LogModule::Net, "idle sweep closed {} sessions (timeout={}s)", closed, SESSION_APP_IDLE_SECONDS);
    }
}
