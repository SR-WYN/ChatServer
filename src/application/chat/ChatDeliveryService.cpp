#include "ChatDeliveryService.h"
#include "CSession.h"
#include "ChatGrpcClient.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"
#include "UserMgr.h"
#include "const.h"
#include "message.pb.h"
#include <iostream>
#include <json/value.h>
#include <string>

bool ChatDeliveryService::deliverTextChat(int from_uid, int to_uid, const Json::Value &text_array,
                                          const Json::Value &return_value)
{
    auto to_str = std::to_string(to_uid);
    auto to_ip_key = RedisPrefix::USERIPPREFIX + to_str;
    std::string to_ip_value;
    if (!RedisMgr::getInstance().get(to_ip_key, to_ip_value))
    {
        return false;
    }

    auto &cfg = ConfigMgr::getInstance();
    auto self_name = cfg["SelfServer"]["Name"];
    if (to_ip_value == self_name)
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

    TextChatMsgRsp rsp =
        ChatGrpcClient::getInstance().NotifyTextChatMsg(to_ip_value, text_msg_req, return_value);
    if (rsp.error() != ErrorCodes::SUCCESS)
    {
        return false;
    }
    return rsp.recipient_online();
}
