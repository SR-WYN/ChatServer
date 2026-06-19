// LoginImpl.cpp
#include "LoginImpl.h"
#include "CSession.h"
#include "MySqlMgr.h"
#include "RuntimeContext.h"
#include "StatusGrpcClient.h"
#include "UserInfoCache.h"
#include "UserSessionManager.h"
#include "const.h"
#include "data.h"
#include "utils.h"
#include <json/reader.h>
#include <json/value.h>
#include <memory>
#include <string>
#include <vector>

LoginImpl::LoginImpl(std::shared_ptr<StatusGrpcClient> status_client,
                     std::shared_ptr<UserInfoCache> user_info_cache,
                     std::shared_ptr<MySqlMgr> mysql_mgr,
                     std::shared_ptr<UserSessionManager> session_manager,
                     const RuntimeContext& runtime_context)
    : _status_client(std::move(status_client)),
      _user_info_cache(std::move(user_info_cache)),
      _mysql_mgr(std::move(mysql_mgr)),
      _session_manager(std::move(session_manager)),
      _runtime_context(runtime_context)
{
}

void LoginImpl::handle(std::shared_ptr<CSession> session, const std::string& msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    auto uid = root["uid"].asInt();
    auto token = root["token"].asString();

    Json::Value return_value;
    bool login_rsp_sent = false;
    utils::Defer defer([&return_value, session, &login_rsp_sent]() {
        if (!login_rsp_sent)
        {
            std::string return_str = return_value.toStyledString();
            session->send(return_str, MSG_CHAT_LOGIN_RSP);
        }
    });

    int token_err = _status_client->validateToken(uid, token);
    if (token_err == ErrorCodes::UID_INVALID)
    {
        return_value["error"] = ErrorCodes::UID_INVALID;
        return;
    }
    if (token_err == ErrorCodes::TOKEN_INVALID)
    {
        return_value["error"] = ErrorCodes::TOKEN_INVALID;
        return;
    }
    if (token_err != ErrorCodes::SUCCESS)
    {
        return_value["error"] = ErrorCodes::RPC_FAILED;
        return;
    }

    return_value["error"] = ErrorCodes::SUCCESS;

    auto user_info = std::make_shared<UserInfo>();
    bool b_base = _user_info_cache->getByUid(uid, user_info);
    if (!b_base)
    {
        return_value["error"] = ErrorCodes::UID_INVALID;
        return;
    }

    return_value["uid"] = uid;
    return_value["pwd"] = user_info->pwd;
    return_value["name"] = user_info->name;
    return_value["email"] = user_info->email;
    return_value["nick"] = user_info->nick;
    return_value["desc"] = user_info->desc;
    return_value["sex"] = user_info->sex;
    return_value["icon"] = user_info->icon;

    std::vector<std::shared_ptr<ApplyInfo>> apply_list;
    bool b_apply = _mysql_mgr->getApplyList(uid, apply_list);
    if (b_apply)
    {
        for (auto& apply : apply_list)
        {
            Json::Value obj;
            obj["name"] = apply->_name;
            obj["uid"] = apply->_uid;
            obj["icon"] = apply->_icon;
            obj["nick"] = apply->_nick;
            obj["sex"] = apply->_sex;
            obj["status"] = apply->_status;
            obj["alias_name"] = apply->alias_name;
            return_value["apply_list"].append(obj);
        }
    }

    std::vector<std::shared_ptr<UserInfo>> friend_list;
    bool b_friend_list = _mysql_mgr->getFriendList(uid, friend_list);
    for (auto& friend_element : friend_list)
    {
        Json::Value obj;
        obj["name"] = friend_element->name;
        obj["uid"] = friend_element->uid;
        obj["icon"] = friend_element->icon;
        obj["nick"] = friend_element->nick;
        obj["sex"] = friend_element->sex;
        obj["desc"] = friend_element->desc;
        obj["alias_name"] = friend_element->alias_name;
        return_value["friend_list"].append(obj);
    }

    const auto& self = _runtime_context.getNodeInfo();
    const std::string& server_name = self.name;
    _status_client->bindUserToNode(uid, server_name);

    session->setUserId(uid);
    _session_manager->setUserSession(uid, session);

    login_rsp_sent = true;
    session->send(return_value.toStyledString(), MSG_CHAT_LOGIN_RSP);
}
