// MySqlMgr.h - 数据库访问接口
#pragma once

#include "data.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class MySqlMgr
{
public:
    virtual ~MySqlMgr() = default;

    // ---- 用户 ----
    virtual std::shared_ptr<UserInfo> getUserInfo(int uid) = 0;
    virtual std::shared_ptr<UserInfo> getUserInfo(const std::string& name) = 0;

    // ---- 好友 ----
    virtual bool addFriendApply(int uid, int touid, const std::string& apply_alias_name) = 0;
    virtual bool getFriendAlias(int self_id, int friend_id, std::string& out_alias) = 0;
    virtual bool getFriendApplyAlias(int from_uid, int to_uid, std::string& out_alias) = 0;
    virtual bool getApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& list,
                              int begin = 0, int limit = 10) = 0;
    virtual bool authFriendApply(int uid, int touid) = 0;
    virtual bool addFriend(int applicant_uid, int accepter_uid,
                           const std::string& alias_applicant_for_accepter,
                           const std::string& alias_accepter_for_applicant) = 0;
    virtual bool getFriendList(int uid, std::vector<std::shared_ptr<UserInfo>>& list) = 0;

    // ---- 聊天消息 ----
    virtual bool saveMessage(const ChatMessageRecord& msg) = 0;
    virtual bool existsByClientMsgId(int from_uid, const std::string& client_msg_id) = 0;
    virtual bool enqueueOffline(uint64_t message_id, int owner_uid) = 0;
    virtual bool fetchOfflineBatch(int owner_uid, int limit, std::vector<ChatMessageRecord>& out,
                                   std::vector<uint64_t>& out_inbox_ids) = 0;
    virtual bool removeOfflineByIds(const std::vector<uint64_t>& inbox_ids) = 0;
    virtual bool queryHistory(int self_uid, int peer_uid, uint64_t before_id, int limit,
                              std::vector<ChatMessageRecord>& out) = 0;
};
