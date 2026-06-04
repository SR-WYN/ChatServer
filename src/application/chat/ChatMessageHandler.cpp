#include "ChatMessageHandler.h"
#include "CSession.h"
#include "ChatGrpcClient.h"
#include "MySqlMgr.h"
#include "PersistWorker.h"
#include "RuntimeContext.h"
#include "StatusGrpcClient.h"
#include "UserMgr.h"
#include "const.h"
#include "message.pb.h"
#include "utils.h"
#include <json/reader.h>
#include <json/value.h>
#include <string>

// ---- 投递相关 ----

namespace
{
// 投递给本机内存中的会话
bool deliverToLocalSession(int from_uid, int to_uid, const Json::Value &text_array)
{
    auto peer_session = UserMgr::getInstance().getSession(to_uid);
    if (!peer_session)
    {
        return false;
    }
    Json::Value notify;
    notify["error"] = ErrorCodes::SUCCESS;
    notify["fromuid"] = from_uid;
    notify["touid"] = to_uid;
    notify["text_array"] = text_array;
    peer_session->send(notify.toStyledString(), MSG_NOTIFY_TEXT_CHAT_MSG_REQ);
    return true;
}
} // namespace

bool ChatMessageHandler::deliverTextChat(int from_uid, int to_uid,
                                         const Json::Value &text_array,
                                         const Json::Value &return_value)
{
    // 本机有会话则直接推送
    if (deliverToLocalSession(from_uid, to_uid, text_array))
    {
        return true;
    }

    auto loc = StatusGrpcClient::getInstance().getUserChatNode(to_uid);
    if (!loc)
    {
        return false;
    }

    const auto &self = RuntimeContext::getInstance().getNodeInfo();
    if (loc->node_name == self.name)
    {
        return false;
    }

    TextChatMsgReq text_msg_req;
    text_msg_req.set_fromuid(from_uid);
    text_msg_req.set_touid(to_uid);
    for (const auto &text_obj : text_array)
    {
        if (!text_obj.isObject())
        {
            continue;
        }
        auto *text_msg = text_msg_req.add_textmsgs();
        text_msg->set_msgid(text_obj["msgid"].asString());
        text_msg->set_msgcontent(text_obj["content"].asString());
    }

    TextChatMsgRsp rsp = ChatGrpcClient::getInstance().NotifyTextChatMsg(
        loc->rpc_host, loc->rpc_port, text_msg_req, return_value);
    if (rsp.error() != ErrorCodes::SUCCESS)
    {
        return false;
    }
    return rsp.recipient_online();
}

// ---- 持久化相关 ----

namespace
{
// 同步持久化单条消息（离线场景）
bool persistOneMessage(int from_uid, int to_uid, const std::string &msgid,
                       const std::string &content, bool delivered_online)
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
        return false;
    }
    if (!delivered_online)
    {
        if (!repo.enqueueOffline(db_id, to_uid))
        {
            return false;
        }
    }
    return true;
}
} // namespace

void ChatMessageHandler::persistOutgoingBatch(int from_uid, int to_uid,
                                              const Json::Value &text_array,
                                              bool delivered_online)
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
            // 离线消息同步落库
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

// ---- 入口 ----

void ChatMessageHandler::handleTextChat(std::shared_ptr<CSession> session, const short &msg_id,
                                        const std::string &msg_data)
{
    (void)msg_id;
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    if (root.isNull() || root.empty())
    {
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

    utils::Defer defer([&return_value, session]() {
        std::string return_str = return_value.toStyledString();
        session->send(return_str, MSG_TEXT_CHAT_MSG_RSP);
    });

    const bool delivered_online =
        deliverTextChat(fromuid, touid, text_array, return_value);
    persistOutgoingBatch(fromuid, touid, text_array, delivered_online);
}
