// MySqlMgrImpl.h - 数据库访问实现
#pragma once

#include "MySqlMgr.h"
#include <memory>

class MySqlPool;

class MySqlMgrImpl final : public MySqlMgr
{
public:
    explicit MySqlMgrImpl(MySqlPool* pool);
    ~MySqlMgrImpl() override = default;

    std::shared_ptr<UserInfo> getUserInfo(int uid) override;
    std::shared_ptr<UserInfo> getUserInfo(const std::string& name) override;

    bool addFriendApply(int uid, int touid, const std::string& apply_alias_name) override;
    bool getFriendAlias(int self_id, int friend_id, std::string& out_alias) override;
    bool getFriendApplyAlias(int from_uid, int to_uid, std::string& out_alias) override;
    bool getApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& list, int begin = 0,
                      int limit = 10) override;
    bool authFriendApply(int uid, int touid) override;
    bool addFriend(int applicant_uid, int accepter_uid,
                   const std::string& alias_applicant_for_accepter,
                   const std::string& alias_accepter_for_applicant) override;
    bool getFriendList(int uid, std::vector<std::shared_ptr<UserInfo>>& list) override;

    bool saveMessage(const ChatMessageRecord& msg) override;
    bool existsByClientMsgId(int from_uid, const std::string& client_msg_id) override;
    bool enqueueOffline(uint64_t message_id, int owner_uid) override;
    bool fetchOfflineBatch(int owner_uid, int limit, std::vector<ChatMessageRecord>& out,
                           std::vector<uint64_t>& out_inbox_ids) override;
    bool removeOfflineByIds(const std::vector<uint64_t>& inbox_ids) override;
    bool queryHistory(int self_uid, int peer_uid, uint64_t before_id, int limit,
                      std::vector<ChatMessageRecord>& out) override;

private:
    MySqlPool* _pool;
};
