#pragma once

#include "data.h"
#include <memory>
#include <string>
#include <vector>

class FriendDao
{
public:
    bool addFriendApply(const int &uid, const int &touid, const std::string &apply_alias_name);
    bool getFriendAlias(int self_id, int friend_id, std::string &out_alias);
    bool getFriendApplyAlias(int from_uid, int to_uid, std::string &out_alias);
    bool getApplyList(const int &touid, std::vector<std::shared_ptr<ApplyInfo>> &list,
                      int begin = 0, int limit = 10);
    bool authFriendApply(const int &uid, const int &touid);
    bool addFriend(int applicant_uid, int accepter_uid,
                   const std::string &alias_applicant_for_accepter,
                   const std::string &alias_accepter_for_applicant);
    bool getFriendList(int uid, std::vector<std::shared_ptr<UserInfo>> &list);
};
