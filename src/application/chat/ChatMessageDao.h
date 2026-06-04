#pragma once

#include "ChatMessage.h"
#include <cstdint>
#include <string>
#include <vector>

// 聊天消息数据访问层：封装 MySQL 读写操作
class ChatMessageDao
{
public:
    bool saveMessage(const ChatMessage &msg, uint64_t &out_db_id);
    bool existsByClientMsgId(int from_uid, const std::string &client_msg_id);
    bool enqueueOffline(uint64_t message_id, int owner_uid);
    bool fetchOfflineBatch(int owner_uid, int limit, std::vector<ChatMessage> &out,
                           std::vector<uint64_t> &out_inbox_ids);
    bool removeOfflineByIds(const std::vector<uint64_t> &inbox_ids);
    bool queryHistory(int self_uid, int peer_uid, uint64_t before_id, int limit,
                    std::vector<ChatMessage> &out);
};
