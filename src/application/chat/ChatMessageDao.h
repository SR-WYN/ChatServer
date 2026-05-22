#pragma once

#include "IChatMessageRepository.h"

class ChatMessageDao : public IChatMessageRepository
{
public:
    bool saveMessage(const ChatMessage &msg, uint64_t &out_db_id) override;
    bool existsByClientMsgId(int from_uid, const std::string &client_msg_id) override;
    bool enqueueOffline(uint64_t message_id, int owner_uid) override;
    bool fetchOfflineBatch(int owner_uid, int limit, std::vector<ChatMessage> &out,
                           std::vector<uint64_t> &out_inbox_ids) override;
    bool removeOfflineByIds(const std::vector<uint64_t> &inbox_ids) override;
    bool queryHistory(int self_uid, int peer_uid, uint64_t before_id, int limit,
                    std::vector<ChatMessage> &out) override;
};
