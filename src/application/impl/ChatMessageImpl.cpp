// ChatMessageImpl.cpp
#include "ChatMessageImpl.h"
#include "CSession.h"
#include <algorithm>
#include <set>
#include "ChatGrpcClient.h"
#include "Log.h"
#include "LogModule.h"
#include "data.h"
#include "IdGenerator.h"
#include "MySqlMgr.h"
#include "RuntimeContext.h"
#include "ThreadPoolMgr.h"
#include "StatusGrpcClient.h"
#include "UserNodeRouteCache.h"
#include "UserSessionManager.h"
#include "const.h"
#include "message.pb.h"
#include "utils.h"
#include <chrono>
#include <json/reader.h>
#include <json/value.h>
#include <string>

struct PersistTask
{
    ChatMessageRecord message;
    bool delivered_online = false;
};

ChatMessageImpl::ChatMessageImpl(std::shared_ptr<UserSessionManager> session_manager,
                                 std::shared_ptr<UserNodeRouteCache> route_cache,
                                 std::shared_ptr<MySqlMgr> mysql_mgr,
                                 std::shared_ptr<StatusGrpcClient> status_client,
                                 std::shared_ptr<ChatGrpcClient> chat_client,
                                 std::shared_ptr<IdGenerator> id_generator,
                                 const RuntimeContext& runtime_context)
    : _session_manager(std::move(session_manager)),
      _route_cache(std::move(route_cache)),
      _mysql_mgr(std::move(mysql_mgr)),
      _status_client(std::move(status_client)),
      _chat_client(std::move(chat_client)),
      _id_generator(std::move(id_generator)),
      _runtime_context(runtime_context)
{
}

bool ChatMessageImpl::deliverToLocalSession(int from_uid, int to_uid,
                                            const Json::Value& text_array)
{
    auto peer_session = _session_manager->getSession(to_uid);
    if (!peer_session)
    {
        LOGD(LogModule::Chat, "deliverToLocalSession: no local session to_uid={}", to_uid);
        return false;
    }
    Json::Value notify;
    notify["error"] = ErrorCodes::SUCCESS;
    notify["fromuid"] = from_uid;
    notify["touid"] = to_uid;
    notify["text_array"] = text_array;
    peer_session->send(notify.toStyledString(), MSG_NOTIFY_TEXT_CHAT_MSG_REQ);
    LOGI(LogModule::Chat, "delivered text locally from_uid={} to_uid={} count={}", from_uid, to_uid,
         text_array.size());
    return true;
}

bool ChatMessageImpl::deliverTextChat(int from_uid, int to_uid, const Json::Value& text_array,
                                      const Json::Value& return_value)
{
    if (deliverToLocalSession(from_uid, to_uid, text_array))
    {
        return true;
    }

    auto cached = _route_cache->get(to_uid);
    std::optional<UserNodeLocation> loc;
    if (cached)
    {
        loc = cached;
        LOGD(LogModule::Chat, "text chat route cache hit to_uid={} node={}", to_uid,
             loc->node_name);
    }
    else
    {
        loc = _status_client->getUserNode(to_uid);
        if (loc)
        {
            _route_cache->put(to_uid, *loc);
            LOGI(LogModule::Chat, "text chat resolved route to_uid={} node={}", to_uid,
                 loc->node_name);
        }
    }

    if (!loc)
    {
        LOGW(LogModule::Chat, "text chat: no route to_uid={}", to_uid);
        return false;
    }

    const auto& self = _runtime_context.getNodeInfo();
    if (loc->node_name == self.name)
    {
        LOGD(LogModule::Chat, "text chat: target on same node but not online to_uid={}", to_uid);
        return false;
    }

    message::TextChatMsgReq text_msg_req;
    text_msg_req.set_fromuid(from_uid);
    text_msg_req.set_touid(to_uid);
    for (const auto& text_obj : text_array)
    {
        if (!text_obj.isObject())
        {
            continue;
        }
        auto* text_msg = text_msg_req.add_textmsgs();
        text_msg->set_msgid(text_obj["msgid"].asString());
        text_msg->set_msgcontent(text_obj["content"].asString());
    }

    message::TextChatMsgRsp rsp =
        _chat_client->NotifyTextChatMsg(loc->rpc_host, loc->rpc_port, text_msg_req, return_value);
    if (rsp.error() != ErrorCodes::SUCCESS)
    {
        LOGW(LogModule::Chat, "text chat cross-node failed to_uid={} node={} error={}", to_uid,
             loc->node_name, rsp.error());
        return false;
    }
    LOGI(LogModule::Chat, "text chat cross-node delivered to_uid={} node={} recipient_online={}",
         to_uid, loc->node_name, rsp.recipient_online());
    return rsp.recipient_online();
}

void ChatMessageImpl::persistOutgoingBatch(int from_uid, int to_uid,
                                           const Json::Value& text_array, bool delivered_online)
{
    if (!text_array.isArray() || text_array.empty())
    {
        return;
    }

    for (const auto& text_obj : text_array)
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

        PersistTask task;
        task.message.id = _id_generator->nextId();
        task.message.client_msg_id = msgid;
        task.message.from_uid = from_uid;
        task.message.to_uid = to_uid;
        task.message.content = content;
        task.delivered_online = delivered_online;

        auto mysql_mgr = _mysql_mgr;
        ThreadPoolMgr::getInstance().enqueueMySql(
            [mysql_mgr, task = std::move(task)]() mutable {
                if (mysql_mgr->existsByClientMsgId(task.message.from_uid,
                                                   task.message.client_msg_id))
                {
                    LOGW(LogModule::Chat,
                         "duplicate message ignored | from_uid={} client_msg_id={}",
                         task.message.from_uid, task.message.client_msg_id);
                    return;
                }

                if (!mysql_mgr->saveMessage(task.message))
                {
                    LOGE(LogModule::Chat,
                         "failed to save message | snowflake_id={} from_uid={} to_uid={} "
                         "client_msg_id={}",
                         task.message.id, task.message.from_uid, task.message.to_uid,
                         task.message.client_msg_id);
                    return;
                }

                LOGI(LogModule::Chat,
                     "message persisted | snowflake_id={} from_uid={} to_uid={} client_msg_id={}",
                     task.message.id, task.message.from_uid, task.message.to_uid,
                     task.message.client_msg_id);

                if (!task.delivered_online)
                {
                    mysql_mgr->enqueueOffline(task.message.id, task.message.to_uid);
                    LOGI(LogModule::Chat, "offline message enqueued | snowflake_id={} to_uid={}",
                         task.message.id, task.message.to_uid);
                }
            });
    }
}

void ChatMessageImpl::handleTextChat(std::shared_ptr<CSession> session, const std::string& msg_data)
{
    const auto start = std::chrono::steady_clock::now();
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    if (root.isNull() || root.empty())
    {
        LOGW(LogModule::Chat, "handleTextChat: invalid JSON session={}", session->getSessionId());
        return;
    }
    auto fromuid = root["fromuid"].asInt();
    auto touid = root["touid"].asInt();
    const Json::Value text_array = root["text_array"];
    Json::Value return_value;
    return_value["error"] = ErrorCodes::SUCCESS;
    return_value["fromuid"] = touid;
    return_value["touid"] = fromuid;
    return_value["text_array"] = text_array;

    LOGI(LogModule::Chat, "handleTextChat from_uid={} to_uid={} count={}", fromuid, touid,
         text_array.size());

    utils::Defer defer([&return_value, session, fromuid, touid]() {
        std::string return_str = return_value.toStyledString();
        LOGI(LogModule::Chat, "text chat ack from_uid={} to_uid={} error={}", fromuid, touid,
             return_value["error"].asInt());
        session->send(return_str, MSG_TEXT_CHAT_MSG_RSP);
    });

    const bool delivered_online = deliverTextChat(fromuid, touid, text_array, return_value);
    persistOutgoingBatch(fromuid, touid, text_array, delivered_online);

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    LOGI(LogModule::Chat, "handleTextChat done from_uid={} to_uid={} delivered_online={} cost={}ms",
         fromuid, touid, delivered_online, cost_ms);
}

namespace
{
constexpr int kOfflineBatchLimit = 100;

std::string compactJson(const Json::Value& value)
{
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, value);
}

Json::Value chatMsgToJson(const ChatMessageRecord& msg)
{
    Json::Value element;
    element["msgid"] = msg.client_msg_id;
    element["content"] = msg.content;
    element["fromuid"] = msg.from_uid;
    element["touid"] = msg.to_uid;
    element["snowflake_id"] = static_cast<Json::UInt64>(msg.id);
    element["msg_type"] = msg.msg_type;
    if (msg.msg_type == 1)
    {
        element["url"] = msg.content;
    }
    return element;
}
} // namespace

void ChatMessageImpl::handleHistory(std::shared_ptr<CSession> session,
                                    const std::string& msg_data)
{
    const auto start = std::chrono::steady_clock::now();
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
        LOGW(LogModule::Chat, "handleHistory: invalid JSON session={}", session->getSessionId());
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
        LOGW(LogModule::Chat, "handleHistory: invalid uid session={}", session->getSessionId());
        result["error"] = ErrorCodes::TOKEN_INVALID;
        return;
    }

    if (!root.isMember("peer_uid"))
    {
        LOGW(LogModule::Chat, "handleHistory: missing peer_uid session={} uid={}",
             session->getSessionId(), self_uid);
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

    LOGI(LogModule::Chat, "handleHistory self_uid={} peer_uid={} before_id={} limit={}", self_uid,
         peer_uid, before_id, limit);

    std::vector<ChatMessageRecord> messages;
    if (!_mysql_mgr->queryHistory(self_uid, peer_uid, before_id, limit, messages))
    {
        LOGE(LogModule::Chat, "handleHistory: queryHistory failed self_uid={} peer_uid={}",
             self_uid, peer_uid);
        result["error"] = ErrorCodes::RPC_FAILED;
        return;
    }
    LOGI(LogModule::Chat, "handleHistory: loaded history count={} self_uid={} peer_uid={}",
         messages.size(), self_uid, peer_uid);

    if (before_id == 0)
    {
        std::vector<ChatMessageRecord> offline_msgs;
        std::vector<uint64_t> inbox_ids;
        if (_mysql_mgr->fetchOfflineBatch(self_uid, kOfflineBatchLimit, offline_msgs, inbox_ids))
        {
            if (!inbox_ids.empty())
            {
                _mysql_mgr->removeOfflineByIds(inbox_ids);
                LOGI(LogModule::Chat,
                     "handleHistory: removed offline inbox ids count={} self_uid={}",
                     inbox_ids.size(), self_uid);
            }

            std::set<uint64_t> seen_ids;
            for (const auto& msg : messages)
            {
                seen_ids.insert(msg.id);
            }
            for (const auto& msg : offline_msgs)
            {
                if (seen_ids.find(msg.id) == seen_ids.end())
                {
                    messages.push_back(msg);
                    seen_ids.insert(msg.id);
                }
            }

            std::sort(messages.begin(), messages.end(),
                      [](const ChatMessageRecord& a, const ChatMessageRecord& b) {
                          return a.id < b.id;
                      });
            LOGI(LogModule::Chat, "handleHistory: merged offline count={} self_uid={}",
                 offline_msgs.size(), self_uid);
        }
    }

    result["peer_uid"] = peer_uid;
    Json::Value text_array(Json::arrayValue);
    for (const auto& msg : messages)
    {
        text_array.append(chatMsgToJson(msg));
    }
    result["text_array"] = text_array;

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    LOGI(LogModule::Chat,
         "handleHistory done self_uid={} peer_uid={} total={} cost={}ms", self_uid, peer_uid,
         messages.size(), cost_ms);
}

bool ChatMessageImpl::deliverImageToLocalSession(int from_uid, int to_uid,
                                                 const Json::Value& image_array)
{
    auto peer_session = _session_manager->getSession(to_uid);
    if (!peer_session)
    {
        LOGD(LogModule::Chat, "deliverImageToLocalSession: no local session to_uid={}", to_uid);
        return false;
    }
    Json::Value notify;
    notify["error"] = ErrorCodes::SUCCESS;
    notify["fromuid"] = from_uid;
    notify["touid"] = to_uid;
    notify["image_array"] = image_array;
    peer_session->send(notify.toStyledString(), MSG_NOTIFY_IMAGE_CHAT_MSG_REQ);
    LOGI(LogModule::Chat, "delivered image locally from_uid={} to_uid={} count={}", from_uid, to_uid,
         image_array.size());
    return true;
}

bool ChatMessageImpl::deliverImageChat(int from_uid, int to_uid,
                                       const Json::Value& image_array)
{
    if (deliverImageToLocalSession(from_uid, to_uid, image_array))
    {
        return true;
    }

    auto cached = _route_cache->get(to_uid);
    std::optional<UserNodeLocation> loc;
    if (cached)
    {
        loc = cached;
        LOGD(LogModule::Chat, "image chat route cache hit to_uid={} node={}", to_uid,
             loc->node_name);
    }
    else
    {
        loc = _status_client->getUserNode(to_uid);
        if (loc)
        {
            _route_cache->put(to_uid, *loc);
            LOGI(LogModule::Chat, "image chat resolved route to_uid={} node={}", to_uid,
                 loc->node_name);
        }
    }

    if (!loc)
    {
        LOGW(LogModule::Chat, "image chat: no route to_uid={}", to_uid);
        return false;
    }

    const auto& self = _runtime_context.getNodeInfo();
    if (loc->node_name == self.name)
    {
        LOGD(LogModule::Chat, "image chat: target on same node but not online to_uid={}", to_uid);
        return false;
    }

    message::ImageChatMsgReq image_msg_req;
    image_msg_req.set_fromuid(from_uid);
    image_msg_req.set_touid(to_uid);
    for (const auto& img_obj : image_array)
    {
        if (!img_obj.isObject())
        {
            continue;
        }
        auto* img_msg = image_msg_req.add_imagemsgs();
        img_msg->set_msgid(img_obj["msgid"].asString());
        img_msg->set_url(img_obj["url"].asString());
        img_msg->set_width(img_obj["width"].asInt());
        img_msg->set_height(img_obj["height"].asInt());
        img_msg->set_size(static_cast<int64_t>(img_obj["size"].asInt64()));
        img_msg->set_filename(img_obj["filename"].asString());
    }

    message::ImageChatMsgRsp rsp =
        _chat_client->NotifyImageChatMsg(loc->rpc_host, loc->rpc_port, image_msg_req, Json::Value());
    if (rsp.error() != ErrorCodes::SUCCESS)
    {
        LOGW(LogModule::Chat, "image chat cross-node failed to_uid={} node={} error={}", to_uid,
             loc->node_name, rsp.error());
        return false;
    }
    LOGI(LogModule::Chat, "image chat cross-node delivered to_uid={} node={}", to_uid,
         loc->node_name);
    return true;
}

void ChatMessageImpl::persistImageBatch(int from_uid, int to_uid,
                                        const Json::Value& image_array, bool delivered_online)
{
    if (!image_array.isArray() || image_array.empty())
    {
        return;
    }

    for (const auto& img_obj : image_array)
    {
        if (!img_obj.isObject())
        {
            continue;
        }
        const std::string msgid = img_obj.isMember("msgid") ? img_obj["msgid"].asString() : "";
        const std::string url = img_obj.isMember("url") ? img_obj["url"].asString() : "";
        if (msgid.empty() || url.empty())
        {
            continue;
        }

        PersistTask task;
        task.message.id = _id_generator->nextId();
        task.message.client_msg_id = msgid;
        task.message.from_uid = from_uid;
        task.message.to_uid = to_uid;
        task.message.content = url;
        task.message.msg_type = 1; // 图片消息
        task.delivered_online = delivered_online;

        auto mysql_mgr = _mysql_mgr;
        ThreadPoolMgr::getInstance().enqueueMySql(
            [mysql_mgr, task = std::move(task)]() mutable {
                if (mysql_mgr->existsByClientMsgId(task.message.from_uid,
                                                   task.message.client_msg_id))
                {
                    LOGW(LogModule::Chat,
                         "duplicate image ignored | from_uid={} client_msg_id={}",
                         task.message.from_uid, task.message.client_msg_id);
                    return;
                }

                if (!mysql_mgr->saveMessage(task.message))
                {
                    LOGE(LogModule::Chat,
                         "failed to save image | snowflake_id={} from_uid={} to_uid={} "
                         "client_msg_id={}",
                         task.message.id, task.message.from_uid, task.message.to_uid,
                         task.message.client_msg_id);
                    return;
                }

                LOGI(LogModule::Chat,
                     "image persisted | snowflake_id={} from_uid={} to_uid={} client_msg_id={}",
                     task.message.id, task.message.from_uid, task.message.to_uid,
                     task.message.client_msg_id);

                if (!task.delivered_online)
                {
                    mysql_mgr->enqueueOffline(task.message.id, task.message.to_uid);
                    LOGI(LogModule::Chat, "offline image enqueued | snowflake_id={} to_uid={}",
                         task.message.id, task.message.to_uid);
                }
            });
    }
}

void ChatMessageImpl::handleImageChat(std::shared_ptr<CSession> session,
                                      const std::string& msg_data)
{
    const auto start = std::chrono::steady_clock::now();
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    if (root.isNull() || root.empty())
    {
        LOGW(LogModule::Chat, "handleImageChat: invalid JSON session={}", session->getSessionId());
        return;
    }
    auto fromuid = root["fromuid"].asInt();
    auto touid = root["touid"].asInt();
    const Json::Value image_array = root["image_array"];
    Json::Value return_value;
    return_value["error"] = ErrorCodes::SUCCESS;
    return_value["fromuid"] = touid;
    return_value["touid"] = fromuid;
    return_value["image_array"] = image_array;

    LOGI(LogModule::Chat, "handleImageChat from_uid={} to_uid={} count={}", fromuid, touid,
         image_array.size());

    utils::Defer defer([&return_value, session, fromuid, touid]() {
        std::string return_str = return_value.toStyledString();
        LOGI(LogModule::Chat, "image chat ack from_uid={} to_uid={} error={}", fromuid, touid,
             return_value["error"].asInt());
        session->send(return_str, MSG_IMAGE_CHAT_MSG_RSP);
    });

    const bool delivered_online = deliverImageChat(fromuid, touid, image_array);
    persistImageBatch(fromuid, touid, image_array, delivered_online);

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    LOGI(LogModule::Chat, "handleImageChat done from_uid={} to_uid={} delivered_online={} cost={}ms",
         fromuid, touid, delivered_online, cost_ms);
}
