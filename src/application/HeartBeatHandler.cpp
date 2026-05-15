#include "HeartBeatHandler.h"
#include "CSession.h"
#include "const.h"
#include <json/value.h>
#include <json/writer.h>

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
    (void)session;
    // TODO: 更新会话最后活跃时间，供空闲踢线
}

void HeartBeatHandler::scheduleIdleSweep()
{
    // TODO: 定时扫描长时间无心跳的连接并 close
}
