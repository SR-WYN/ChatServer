#include "ChatDeliveryService.h"
#include "CSession.h"
#include "ChatGrpcClient.h"
#include "ChatRuntimeConfig.h"
#include "StatusGrpcClient.h"
#include "UserMgr.h"
#include "const.h"
#include "message.pb.h"
#include <json/value.h>
#include <string>

namespace
{
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

bool ChatDeliveryService::deliverTextChat(int from_uid, int to_uid, const Json::Value &text_array,
                                          const Json::Value &return_value)
{
    // 本机有会话则直接推送（不依赖 Status 绑定，避免 bind 失败时误判离线）
    if (deliverToLocalSession(from_uid, to_uid, text_array))
    {
        return true;
    }

    auto loc = StatusGrpcClient::getInstance().getUserChatNode(to_uid);
    if (!loc)
    {
        return false;
    }

    const auto &self = ChatRuntimeConfig::getInstance().self();
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
