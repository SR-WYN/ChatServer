// FriendImpl.h - 好友业务实现
#pragma once

#include "Friend.h"
#include <memory>

class ChatGrpcClient;
class MySqlMgr;
class RuntimeContext;
class StatusGrpcClient;
class UserInfoCache;
class UserNodeRouteCache;
class UserSessionManager;

class FriendImpl final : public Friend
{
public:
    FriendImpl(std::shared_ptr<StatusGrpcClient> status_client,
               std::shared_ptr<ChatGrpcClient> chat_client,
               std::shared_ptr<UserSessionManager> session_manager,
               std::shared_ptr<UserNodeRouteCache> route_cache,
               std::shared_ptr<MySqlMgr> mysql_mgr,
               std::shared_ptr<UserInfoCache> user_info_cache,
               const RuntimeContext& runtime_context);

    void handleAddFriend(std::shared_ptr<CSession> session, const std::string& msg_data) override;
    void handleAuthFriend(std::shared_ptr<CSession> session, const std::string& msg_data) override;

private:
    std::shared_ptr<StatusGrpcClient> _status_client;
    std::shared_ptr<ChatGrpcClient> _chat_client;
    std::shared_ptr<UserSessionManager> _session_manager;
    std::shared_ptr<UserNodeRouteCache> _route_cache;
    std::shared_ptr<MySqlMgr> _mysql_mgr;
    std::shared_ptr<UserInfoCache> _user_info_cache;
    const RuntimeContext& _runtime_context;
};
