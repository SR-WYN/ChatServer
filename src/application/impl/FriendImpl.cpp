// FriendImpl.cpp
#include "FriendImpl.h"
#include "CSession.h"
#include "ChatGrpcClient.h"
#include "MySqlMgr.h"
#include "RuntimeContext.h"
#include "StatusGrpcClient.h"
#include "UserInfoCache.h"
#include "UserSessionManager.h"
#include "UserNodeRouteCache.h"
#include "const.h"
#include "data.h"
#include "message.pb.h"
#include "utils.h"
#include <json/reader.h>
#include <json/value.h>
#include <memory>
#include <string>

FriendImpl::FriendImpl(std::shared_ptr<StatusGrpcClient> status_client,
                       std::shared_ptr<ChatGrpcClient> chat_client,
                       std::shared_ptr<UserSessionManager> session_manager,
                       std::shared_ptr<UserNodeRouteCache> route_cache,
                       std::shared_ptr<MySqlMgr> mysql_mgr,
                       std::shared_ptr<UserInfoCache> user_info_cache,
                       const RuntimeContext& runtime_context)
    : _status_client(std::move(status_client)),
      _chat_client(std::move(chat_client)),
      _session_manager(std::move(session_manager)),
      _route_cache(std::move(route_cache)),
      _mysql_mgr(std::move(mysql_mgr)),
      _user_info_cache(std::move(user_info_cache)),
      _runtime_context(runtime_context)
{
}

void FriendImpl::handleAddFriend(std::shared_ptr<CSession> session, const std::string& msg_data)
{
    (void)session;
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    auto uid = root["uid"].asInt();
    auto name = root["apply_name"].asString();
    auto alias_name = root["alias_name"].asString();
    auto touid = root["touid"].asInt();

    Json::Value return_value;
    return_value["error"] = ErrorCodes::SUCCESS;
    utils::Defer defer([&return_value, session]() {
        std::string return_str = return_value.toStyledString();
        session->send(return_str, MSG_ADD_FRIEND_RSP);
    });

    _mysql_mgr->addFriendApply(uid, touid, alias_name);

    auto peer_loc = _route_cache->get(touid);
    if (!peer_loc)
    {
        peer_loc = _status_client->getUserChatNode(touid);
        if (peer_loc)
        {
            _route_cache->put(touid, *peer_loc);
        }
    }

    if (!peer_loc)
    {
        return;
    }

    auto apply_info = std::make_shared<UserInfo>();
    bool b_info = _user_info_cache->getByUid(uid, apply_info);

    const auto& self_name = _runtime_context.getNodeInfo().name;
    if (peer_loc->node_name == self_name)
    {
        auto to_user_session = _session_manager->getSession(touid);
        if (to_user_session)
        {
            Json::Value notify;
            notify["error"] = ErrorCodes::SUCCESS;
            notify["applyuid"] = uid;
            notify["name"] = name;
            notify["desc"] = "";
            if (b_info)
            {
                notify["icon"] = apply_info->icon;
                notify["nick"] = apply_info->nick;
                notify["sex"] = apply_info->sex;
            }
            else
            {
                notify["icon"] = "";
                notify["nick"] = "";
                notify["sex"] = 0;
            }
            notify["alias_name"] = alias_name;
            std::string return_str = notify.toStyledString();
            to_user_session->send(return_str, MSG_NOTIFY_ADDFRIEND_REQ);
        }
        return;
    }

    message::AddFriendReq add_req;
    add_req.set_applyuid(uid);
    add_req.set_name(name);
    add_req.set_desc("");
    add_req.set_touid(touid);
    if (b_info)
    {
        add_req.set_icon(apply_info->icon.c_str());
        add_req.set_nick(apply_info->nick.c_str());
        add_req.set_sex(apply_info->sex);
    }
    add_req.set_alias_name(alias_name);
    _chat_client->NotifyAddFriend(peer_loc->rpc_host, peer_loc->rpc_port, add_req);
}

void FriendImpl::handleAuthFriend(std::shared_ptr<CSession> session, const std::string& msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    const int applicant_uid = root["fromuid"].asInt();
    const int accepter_uid = root["touid"].asInt();
    auto alias_name = root["alias_name"].asString();
    Json::Value return_value;
    return_value["error"] = ErrorCodes::SUCCESS;
    auto user_info = std::make_shared<UserInfo>();

    bool b_info = _user_info_cache->getByUid(applicant_uid, user_info);
    if (b_info)
    {
        return_value["name"] = user_info->name;
        return_value["nick"] = user_info->nick;
        return_value["icon"] = user_info->icon;
        return_value["sex"] = user_info->sex;
        return_value["uid"] = applicant_uid;
        return_value["alias_name"] = alias_name;
    }
    else
    {
        return_value["error"] = ErrorCodes::UID_INVALID;
    }
    utils::Defer defer([&return_value, session] {
        std::string return_str = return_value.toStyledString();
        session->send(return_str, MSG_AUTH_FRIEND_RSP);
    });

    std::string alias_applicant_for_accepter;
    _mysql_mgr->getFriendApplyAlias(applicant_uid, accepter_uid, alias_applicant_for_accepter);

    _mysql_mgr->authFriendApply(applicant_uid, accepter_uid);
    _mysql_mgr->addFriend(applicant_uid, accepter_uid, alias_applicant_for_accepter, alias_name);

    std::string applicant_peer_alias;
    _mysql_mgr->getFriendAlias(applicant_uid, accepter_uid, applicant_peer_alias);

    auto peer_loc = _route_cache->get(applicant_uid);
    if (!peer_loc)
    {
        peer_loc = _status_client->getUserChatNode(applicant_uid);
        if (peer_loc)
        {
            _route_cache->put(applicant_uid, *peer_loc);
        }
    }
    if (!peer_loc)
    {
        return;
    }

    const auto& self_name = _runtime_context.getNodeInfo().name;
    if (peer_loc->node_name == self_name)
    {
        auto peer_session = _session_manager->getSession(applicant_uid);
        if (peer_session)
        {
            Json::Value notify;
            notify["error"] = ErrorCodes::SUCCESS;
            notify["fromuid"] = accepter_uid;
            notify["touid"] = applicant_uid;
            auto peer_info = std::make_shared<UserInfo>();
            bool b_into = _user_info_cache->getByUid(accepter_uid, peer_info);
            if (b_into)
            {
                notify["name"] = peer_info->name;
                notify["nick"] = peer_info->nick;
                notify["icon"] = peer_info->icon;
                notify["sex"] = peer_info->sex;
                notify["alias_name"] = applicant_peer_alias;
            }
            else
            {
                notify["error"] = ErrorCodes::UID_INVALID;
            }
            std::string return_str = notify.toStyledString();
            peer_session->send(return_str, MSG_NOTIFY_AUTH_FRIEND_REQ);
        }
        return;
    }

    message::AuthFriendReq auth_req;
    auth_req.set_fromuid(accepter_uid);
    auth_req.set_touid(applicant_uid);
    _chat_client->NotifyAuthFriend(peer_loc->rpc_host, peer_loc->rpc_port, auth_req);
}
