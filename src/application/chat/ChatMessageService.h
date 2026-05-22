#pragma once

#include "ChatMessage.h"
#include <json/value.h>
#include <string>
#include <vector>

class ChatMessageService
{
public:
    static void persistOutgoingBatch(int from_uid, int to_uid, const Json::Value &text_array,
                                     bool delivered_online);
    static bool pullOfflineForUser(int owner_uid, int limit, std::vector<ChatMessage> &out_messages);
    static bool queryHistory(int self_uid, int peer_uid, uint64_t before_id, int limit,
                             std::vector<ChatMessage> &out_messages);
};
