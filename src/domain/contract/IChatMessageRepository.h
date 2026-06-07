#pragma once

#include "ChatMessage.h"
#include <cstdint>
#include <string>
#include <vector>

// 聊天消息仓储接口：聊天消息的持久化存储
class IChatMessageRepository
{
public:
    virtual ~IChatMessageRepository() = default;

    virtual bool saveMessage(const ChatMessage &msg, uint64_t &out_db_id) = 0;
    virtual bool existsByClientMsgId(int from_uid, const std::string &client_msg_id) = 0;
    virtual bool enqueueOffline(uint64_t message_id, int owner_uid) = 0;
    virtual bool fetchOfflineBatch(int owner_uid, int limit, std::vector<ChatMessage> &out,
                                   std::vector<uint64_t> &out_inbox_ids) = 0;
    virtual bool removeOfflineByIds(const std::vector<uint64_t> &inbox_ids) = 0;
    virtual bool queryHistory(int self_uid, int peer_uid, uint64_t before_id, int limit,
                              std::vector<ChatMessage> &out) = 0;
};
