#include "ChatQueryHandler.h"
#include "ChatMessage.h"
#include "MySqlMgr.h"
#include "const.h"
#include "utils.h"
#include <json/reader.h>
#include <json/value.h>
#include <json/writer.h>
#include <map>
#include <vector>

namespace
{
constexpr int kOfflineBatchLimit = 100;

std::string compactJson(const Json::Value &value)
{
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, value);
}
} // namespace

// ---- 历史消息查询 ----

void ChatQueryHandler::handleHistory(std::shared_ptr<CSession> session, const short &msg_id,
                                     const std::string &msg_data)
{
    (void)msg_id;
    Json::Reader reader;
    Json::Value root;
    Json::Value result;
    result["error"] = ErrorCodes::SUCCESS;
    result["text_array"] = Json::Value(Json::arrayValue);

    utils::Defer defer([&result, session]() {
        session->send(compactJson(result), MSG_CHAT_HISTORY_RSP);
    });

    if (!reader.parse(msg_data, root))
    {
        result["error"] = ErrorCodes::ERROR_JSON;
        return;
    }

    int self_uid = session->getUserId();
    if (self_uid <= 0 && root.isMember("uid"))
    {
        self_uid = root["uid"].asInt();
    }
    if (self_uid <= 0)
    {
        result["error"] = ErrorCodes::TOKEN_INVALID;
        return;
    }

    if (!root.isMember("peer_uid"))
    {
        result["error"] = ErrorCodes::ERROR_JSON;
        return;
    }

    const int peer_uid = root["peer_uid"].asInt();
    uint64_t before_id = 0;
    if (root.isMember("before_id"))
    {
        before_id = static_cast<uint64_t>(root["before_id"].asUInt64());
    }
    int limit = 100;
    if (root.isMember("limit"))
    {
        limit = root["limit"].asInt();
    }

    auto &repo = MySqlMgr::getInstance().chatMessages();
    std::vector<ChatMessage> messages;
    if (!repo.queryHistory(self_uid, peer_uid, before_id, limit, messages))
    {
        result["error"] = ErrorCodes::RPCFAILED;
        return;
    }

    result["peer_uid"] = peer_uid;
    Json::Value text_array(Json::arrayValue);
    for (const auto &msg : messages)
    {
        Json::Value element;
        element["msgid"] = msg.client_msg_id;
        element["content"] = msg.content;
        element["fromuid"] = msg.from_uid;
        element["touid"] = msg.to_uid;
        text_array.append(element);
    }
    result["text_array"] = text_array;
}

// ---- 离线消息同步 ----

void ChatQueryHandler::syncAfterLogin(std::shared_ptr<CSession> session, int uid)
{
    auto &repo = MySqlMgr::getInstance().chatMessages();
    std::vector<ChatMessage> messages;
    std::vector<uint64_t> inbox_ids;
    if (!repo.fetchOfflineBatch(uid, kOfflineBatchLimit, messages, inbox_ids))
    {
        return;
    }
    if (!inbox_ids.empty())
    {
        repo.removeOfflineByIds(inbox_ids);
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
