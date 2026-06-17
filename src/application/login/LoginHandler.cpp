#include "LoginHandler.h"
#include "CSession.h"
#include "ConfigMgr.h"
#include "MySqlMgr.h"
#include "RuntimeContext.h"
#include "ServiceLocator.h"
#include "StatusGrpcClient.h"
#include "UserInfoCache.h"
#include "UserMgr.h"
#include "const.h"
#include "data.h"
#include "utils.h"
#include <iostream>
#include <json/reader.h>
#include <json/value.h>
#include <memory>
#include <string>
#include <vector>

void LoginHandler::handleLogin(std::shared_ptr<CSession> session, const short &msg_id,
                               const std::string &msg_data)
{
    (void)msg_id;
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

    // 通过 StatusServer RPC 验证 Token
    int token_err = StatusGrpcClient::getInstance().validateToken(uid, token);
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
    bool b_base = ServiceLocator::getService<UserInfoCache>()->getByUid(uid, user_info);
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

    // 从数据库获取申请列表
    std::vector<std::shared_ptr<ApplyInfo>> apply_list;
    bool b_apply = getFriendApplyInfo(uid, apply_list);
    if (b_apply)
    {
        for (auto &apply : apply_list)
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
    // 获取好友列表
    std::vector<std::shared_ptr<UserInfo>> friend_list;
    bool b_friend_list = getFriendList(uid, friend_list);
    for (auto &friend_element : friend_list)
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
    const auto &self = RuntimeContext::getInstance().getNodeInfo();
    const std::string &server_name = self.name;
    StatusGrpcClient::getInstance().bindUserToNode(uid, server_name);
    // LOGIN_COUNT 由 StatusServer 在 bindUser 中原子维护，ChatServer 不再直接操作

    // session绑定用户uid
    session->setUserId(uid);

    // uid和session绑定管理，方便以后踢人操作
    UserMgr::getInstance().setUserSession(uid, session);

    login_rsp_sent = true;
    session->send(return_value.toStyledString(), MSG_CHAT_LOGIN_RSP);
    // 移除 syncAfterLogin 调用：离线消息由客户端发起 CHAT_HISTORY_REQ 时合并返回
}

bool LoginHandler::getFriendApplyInfo(int touid, std::vector<std::shared_ptr<ApplyInfo>> &list)
{
    return MySqlMgr::getInstance().friends().getApplyList(touid, list);
}

bool LoginHandler::getFriendList(int uid, std::vector<std::shared_ptr<UserInfo>> &list)
{
    return MySqlMgr::getInstance().friends().getFriendList(uid, list);
}
