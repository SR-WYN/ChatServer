#include "ChatQueryHandler.h"
#include "ChatMessage.h"
#include "MySqlMgr.h"
#include "const.h"
#include "utils.h"
#include <json/reader.h>
#include <json/value.h>
#include <json/writer.h>
#include <algorithm>
#include <map>
#include <set>
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

// 将 ChatMessage 转为 JSON 对象
Json::Value chatMsgToJson(const ChatMessage &msg)
{
    Json::Value element;
    element["msgid"] = msg.client_msg_id;
    element["content"] = msg.content;
    element["fromuid"] = msg.from_uid;
    element["touid"] = msg.to_uid;
    element["snowflake_id"] = static_cast<Json::UInt64>(msg.id);
    return element;
}
} // namespace

// ---- 历史消息查询（合并离线消息） ----

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

    // 首次查询（before_id == 0）时合并离线消息
    if (before_id == 0)
    {
        std::vector<ChatMessage> offline_msgs;
        std::vector<uint64_t> inbox_ids;
        if (repo.fetchOfflineBatch(self_uid, kOfflineBatchLimit, offline_msgs, inbox_ids))
        {
            if (!inbox_ids.empty())
            {
                repo.removeOfflineByIds(inbox_ids);
            }

            // 按雪花 ID 去重合并
            std::set<uint64_t> seen_ids;
            for (const auto &msg : messages)
            {
                seen_ids.insert(msg.id);
            }
            for (const auto &msg : offline_msgs)
            {
                if (seen_ids.find(msg.id) == seen_ids.end())
                {
                    messages.push_back(msg);
                    seen_ids.insert(msg.id);
                }
            }

            // 按雪花 ID 升序排序
            std::sort(messages.begin(), messages.end(),
                      [](const ChatMessage &a, const ChatMessage &b) {
                          return a.id < b.id;
                      });
        }
    }

    result["peer_uid"] = peer_uid;
    Json::Value text_array(Json::arrayValue);
    for (const auto &msg : messages)
    {
        text_array.append(chatMsgToJson(msg));
    }
    result["text_array"] = text_array;
}

// ---- 离线消息同步（已废弃，由 handleHistory 合并处理） ----

void ChatQueryHandler::syncAfterLogin(std::shared_ptr<CSession> session, int uid)
{
    (void)session;
    (void)uid;
    // 不再单独推送离线消息，由客户端发起 CHAT_HISTORY_REQ 时合并返回
}
