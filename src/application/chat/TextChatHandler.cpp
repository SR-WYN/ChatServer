#include "TextChatHandler.h"
#include "ChatDeliveryService.h"
#include "ChatMessageService.h"
#include "CSession.h"
#include "const.h"
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

    const bool delivered_online =
        ChatDeliveryService::deliverTextChat(fromuid, touid, text_array, return_value);
    ChatMessageService::persistOutgoingBatch(fromuid, touid, text_array, delivered_online);
}
