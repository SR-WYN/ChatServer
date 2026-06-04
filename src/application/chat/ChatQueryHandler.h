#pragma once

#include "CSession.h"
#include <memory>
#include <string>

// 聊天查询处理：历史消息 + 离线消息同步
// 合并 ChatHistoryHandler + OfflineSyncService + ChatMessageService 的查询部分
class ChatQueryHandler
{
public:
    // 处理历史消息查询请求
    static void handleHistory(std::shared_ptr<CSession> session, const short &msg_id,
                              const std::string &msg_data);

    // 登录后拉取离线消息并推送
    static void syncAfterLogin(std::shared_ptr<CSession> session, int uid);

private:
    ChatQueryHandler() = delete;
};
