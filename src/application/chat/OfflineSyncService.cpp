#include "OfflineSyncService.h"
#include "ChatMessageService.h"
#include "const.h"
#include <json/value.h>
#include <map>
#include <vector>

namespace
{
constexpr int kOfflineBatchLimit = 100;
}

void OfflineSyncService::syncAfterLogin(std::shared_ptr<CSession> session, int uid)
{
    std::vector<ChatMessage> messages;
    if (!ChatMessageService::pullOfflineForUser(uid, kOfflineBatchLimit, messages))
    {
        return;
    }

    std::map<int, Json::Value> by_sender;
    for (const auto &msg : messages)
    {
        Json::Value element;
        element["msgid"] = msg.client_msg_id;
        element["content"] = msg.content;
        by_sender[msg.from_uid].append(element);
    }

    for (const auto &entry : by_sender)
    {
        Json::Value notify;
        notify["error"] = ErrorCodes::SUCCESS;
        notify["fromuid"] = entry.first;
        notify["touid"] = uid;
        notify["text_array"] = entry.second;
        session->send(notify.toStyledString(), MSG_NOTIFY_TEXT_CHAT_MSG_REQ);
    }
}
