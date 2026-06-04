#pragma once

#include <json/value.h>
#include <memory>
#include <string>

class CSession;

// 文本聊天消息处理：投递 + 持久化
// 合并 TextChatHandler + ChatDeliveryService + ChatMessageService 的写入部分
class ChatMessageHandler
{
public:
    // 处理文本聊天消息（入口：由 LogicSystem 回调）
    static void handleTextChat(std::shared_ptr<CSession> session, const short &msg_id,
                               const std::string &msg_data);

private:
    ChatMessageHandler() = delete;

    // 投递消息给接收方（本机或跨节点）
    static bool deliverTextChat(int from_uid, int to_uid, const Json::Value &text_array,
                                const Json::Value &return_value);

    // 批量持久化消息（在线异步 / 离线同步）
    static void persistOutgoingBatch(int from_uid, int to_uid, const Json::Value &text_array,
                                     bool delivered_online);
};
