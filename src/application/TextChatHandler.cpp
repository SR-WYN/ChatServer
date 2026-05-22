#include "TextChatHandler.h"
#include "CSession.h"
#include "ChatGrpcClient.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"
#include "UserMgr.h"
#include "const.h"
#include "message.pb.h"
#include "utils.h"
#include <iostream>
#include <json/reader.h>
#include <json/value.h>
#include <memory>
#include <string>

void TextChatHandler::handleTextChat(std::shared_ptr<CSession> session, const short &msg_id,
                                     const std::string &msg_data)
{
    (void)msg_id;
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    if (root.isNull() || root.empty())
    {
        std::cout << "chat text msg data is null or empty" << std::endl;
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

    // 查询redis 查找touid对应的server ip
    auto to_str = std::to_string(touid);
    auto to_ip_key = RedisPrefix::USERIPPREFIX + to_str;
    std::string to_ip_value = "";
    bool b_ip = RedisMgr::getInstance().get(to_ip_key, to_ip_value);
    if (!b_ip)
    {
        return;
    }
    auto &cfg = ConfigMgr::getInstance();
    auto self_name = cfg["SelfServer"]["Name"];
    if (to_ip_value == self_name)
    {
        auto peer_session = UserMgr::getInstance().getSession(touid);
        if (peer_session)
        {
            std::string return_str = return_value.toStyledString();
            peer_session->send(return_str, MSG_TEXT_CHAT_MSG_RSP);
        }
        return;
    }

    TextChatMsgReq text_msg_req;
    text_msg_req.set_fromuid(fromuid);
    text_msg_req.set_touid(touid);
    for (const auto &text_obj : text_array)
    {
        auto content = text_obj["content"].asString();
        auto msgid = text_obj["msgid"].asString();
        std::cout << "content is " << content << " msgid is " << msgid << std::endl;
        auto *text_msg = text_msg_req.add_textmsgs();
        text_msg->set_msgid(msgid);
        text_msg->set_msgcontent(content);
    }
    ChatGrpcClient::getInstance().NotifyTextChatMsg(to_ip_value, text_msg_req, return_value);
}
