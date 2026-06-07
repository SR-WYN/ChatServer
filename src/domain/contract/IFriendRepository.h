#pragma once

#include "data.h"
#include <memory>
#include <string>
#include <vector>

// 好友仓储接口：好友关系与申请的持久化存储
class IFriendRepository
{
public:
    virtual ~IFriendRepository() = default;

    virtual bool addFriendApply(const int &uid, const int &touid,
                                const std::string &apply_alias_name) = 0;
    virtual bool getFriendAlias(int self_id, int friend_id, std::string &out_alias) = 0;
    virtual bool getFriendApplyAlias(int from_uid, int to_uid, std::string &out_alias) = 0;
    virtual bool getApplyList(const int &touid, std::vector<std::shared_ptr<ApplyInfo>> &list,
                              int begin = 0, int limit = 10) = 0;
    virtual bool authFriendApply(const int &uid, const int &touid) = 0;
    virtual bool addFriend(int applicant_uid, int accepter_uid,
                           const std::string &alias_applicant_for_accepter,
                           const std::string &alias_accepter_for_applicant) = 0;
    virtual bool getFriendList(int uid, std::vector<std::shared_ptr<UserInfo>> &list) = 0;
};
