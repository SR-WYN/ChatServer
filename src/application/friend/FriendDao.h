#pragma once

#include "IFriendRepository.h"
#include "data.h"
#include <memory>
#include <string>
#include <vector>

class FriendDao : public IFriendRepository
{
public:
    bool addFriendApply(const int &uid, const int &touid, const std::string &apply_alias_name) override;
    bool getFriendAlias(int self_id, int friend_id, std::string &out_alias) override;
    bool getFriendApplyAlias(int from_uid, int to_uid, std::string &out_alias) override;
    bool getApplyList(const int &touid, std::vector<std::shared_ptr<ApplyInfo>> &list,
                      int begin = 0, int limit = 10) override;
    bool authFriendApply(const int &uid, const int &touid) override;
    bool addFriend(int applicant_uid, int accepter_uid,
                   const std::string &alias_applicant_for_accepter,
                   const std::string &alias_accepter_for_applicant) override;
    bool getFriendList(int uid, std::vector<std::shared_ptr<UserInfo>> &list) override;
};
