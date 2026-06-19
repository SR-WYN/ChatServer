// LoginImpl.h - 登录业务实现
#pragma once

#include "Login.h"
#include <memory>

class MySqlMgr;
class RuntimeContext;
class StatusGrpcClient;
class UserInfoCache;
class UserSessionManager;

class LoginImpl final : public Login
{
public:
    LoginImpl(std::shared_ptr<StatusGrpcClient> status_client,
              std::shared_ptr<UserInfoCache> user_info_cache,
              std::shared_ptr<MySqlMgr> mysql_mgr,
              std::shared_ptr<UserSessionManager> session_manager,
              const RuntimeContext& runtime_context);

    void handle(std::shared_ptr<CSession> session, const std::string& msg_data) override;

private:
    std::shared_ptr<StatusGrpcClient> _status_client;
    std::shared_ptr<UserInfoCache> _user_info_cache;
    std::shared_ptr<MySqlMgr> _mysql_mgr;
    std::shared_ptr<UserSessionManager> _session_manager;
    const RuntimeContext& _runtime_context;
};
