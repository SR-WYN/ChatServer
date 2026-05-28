#include "ChatMessageService.h"
#include "MySqlMgr.h"
#include "PersistWorker.h"
#include <iostream>

namespace
{
bool persistOneMessage(int from_uid, int to_uid, const std::string &msgid, const std::string &content,
                       bool delivered_online)
{
    auto &repo = MySqlMgr::getInstance().chatMessages();
    if (repo.existsByClientMsgId(from_uid, msgid))
    {
        return true;
    }

    ChatMessage msg;
    msg.client_msg_id = msgid;
    msg.from_uid = from_uid;
    msg.to_uid = to_uid;
    msg.content = content;
    uint64_t db_id = 0;
    if (!repo.saveMessage(msg, db_id))
    {
        std::cerr << "persistOneMessage: saveMessage failed msgid=" << msgid << std::endl;
        return false;
    }
    if (!delivered_online)
    {
        if (!repo.enqueueOffline(db_id, to_uid))
        {
            std::cerr << "persistOneMessage: enqueueOffline failed msgid=" << msgid << std::endl;
            return false;
        }
    }
    return true;
}
} // namespace

void ChatMessageService::persistOutgoingBatch(int from_uid, int to_uid,
                                              const Json::Value &text_array, bool delivered_online)
{
    if (!text_array.isArray() || text_array.empty())
    {
        return;
    }

    for (const auto &text_obj : text_array)
    {
        if (!text_obj.isObject())
        {
            continue;
        }
        const std::string msgid = text_obj.isMember("msgid") ? text_obj["msgid"].asString() : "";
        const std::string content =
            text_obj.isMember("content") ? text_obj["content"].asString() : "";
        if (msgid.empty())
        {
            continue;
        }

        if (!delivered_online)
        {
            // 离线消息同步落库，避免登录时 pullOffline 早于异步队列写入
            persistOneMessage(from_uid, to_uid, msgid, content, false);
            continue;
        }

        PersistTask task;
        task.message.client_msg_id = msgid;
        task.message.from_uid = from_uid;
        task.message.to_uid = to_uid;
        task.message.content = content;
        task.delivered_online = true;
        PersistWorker::getInstance().enqueue(std::move(task));
    }
}

bool ChatMessageService::queryHistory(int self_uid, int peer_uid, uint64_t before_id, int limit,
                                      std::vector<ChatMessage> &out_messages)
{
    return MySqlMgr::getInstance().chatMessages().queryHistory(self_uid, peer_uid, before_id,
                                                                 limit, out_messages);
}

bool ChatMessageService::pullOfflineForUser(int owner_uid, int limit,
                                            std::vector<ChatMessage> &out_messages)
{
    out_messages.clear();
    std::vector<uint64_t> inbox_ids;
    auto &repo = MySqlMgr::getInstance().chatMessages();
    if (!repo.fetchOfflineBatch(owner_uid, limit, out_messages, inbox_ids))
    {
        return false;
    }
    if (!inbox_ids.empty())
    {
        repo.removeOfflineByIds(inbox_ids);
    }
    return true;
}
