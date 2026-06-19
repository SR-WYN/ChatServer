// ChatGrpcServiceImpl.cpp
#include "ChatGrpcServiceImpl.h"
#include "CSession.h"
#include "const.h"
#include "message.pb.h"
#include "utils.h"
#include <json/json.h>
#include <json/reader.h>
#include <memory>

ChatGrpcServiceImpl::ChatGrpcServiceImpl(std::shared_ptr<UserSessionManager> session_manager,
                                         std::shared_ptr<UserInfoCache> user_info_cache,
                                         std::shared_ptr<MySqlMgr> mysql_mgr)
    : _session_manager(std::move(session_manager)),
      _user_info_cache(std::move(user_info_cache)),
      _mysql_mgr(std::move(mysql_mgr))
{
}

Status ChatGrpcServiceImpl::NotifyAddFriend(ServerContext* context, const AddFriendReq* request,
                                            AddFriendRsp* reply)
{
    (void)context;
    auto touid = request->touid();
    auto session = _session_manager->getSession(touid);

    utils::Defer defer([request, reply]() {
        reply->set_error(ErrorCodes::SUCCESS);
        reply->set_applyuid(request->applyuid());
        reply->set_touid(request->touid());
    });

    if (session == nullptr)
    {
        return Status::OK;
    }

    Json::Value notify;
    notify["error"] = ErrorCodes::SUCCESS;
    notify["applyuid"] = request->applyuid();
    notify["name"] = request->name();
    notify["desc"] = request->desc();
    notify["icon"] = request->icon();
    notify["sex"] = request->sex();
    notify["nick"] = request->nick();
    notify["alias_name"] = request->alias_name();
    std::string return_str = notify.toStyledString();
    session->send(return_str, MSG_NOTIFY_ADDFRIEND_REQ);
    return Status::OK;
}

Status ChatGrpcServiceImpl::NotifyAuthFriend(ServerContext* context, const AuthFriendReq* request,
                                             AuthFriendRsp* reply)
{
    (void)context;
    auto touid = request->touid();
    auto fromuid = request->fromuid();
    auto session = _session_manager->getSession(touid);

    utils::Defer defer([request, reply]() {
        reply->set_error(ErrorCodes::SUCCESS);
        reply->set_fromuid(request->fromuid());
        reply->set_touid(request->touid());
    });

    if (session == nullptr)
    {
        return Status::OK;
    }

    Json::Value return_value;
    return_value["error"] = ErrorCodes::SUCCESS;
    return_value["fromuid"] = fromuid;
    return_value["touid"] = touid;

    auto user_info = std::make_shared<UserInfo>();
    bool b_info = _user_info_cache->getByUid(fromuid, user_info);
    if (b_info)
    {
        return_value["name"] = user_info->name;
        return_value["nick"] = user_info->nick;
        return_value["icon"] = user_info->icon;
        return_value["sex"] = user_info->sex;
        std::string peer_alias;
        _mysql_mgr->getFriendAlias(touid, fromuid, peer_alias);
        return_value["alias_name"] = peer_alias;
    }
    else
    {
        return_value["error"] = ErrorCodes::UID_INVALID;
    }
    std::string return_str = return_value.toStyledString();
    session->send(return_str, MSG_NOTIFY_AUTH_FRIEND_REQ);
    return Status::OK;
}

Status ChatGrpcServiceImpl::NotifyTextChatMsg(ServerContext* context,
                                              const TextChatMsgReq* request,
                                              TextChatMsgRsp* reply)
{
    (void)context;
    auto touid = request->touid();
    auto session = _session_manager->getSession(touid);
    reply->set_error(ErrorCodes::SUCCESS);
    reply->set_recipient_online(session != nullptr);

    if (session == nullptr)
    {
        return Status::OK;
    }

    Json::Value notify;
    notify["error"] = ErrorCodes::SUCCESS;
    notify["fromuid"] = request->fromuid();
    notify["touid"] = request->touid();

    Json::Value text_array;
    for (const auto& text_data : request->textmsgs())
    {
        Json::Value element;
        element["msgid"] = text_data.msgid();
        element["content"] = text_data.msgcontent();
        text_array.append(element);
    }
    notify["text_array"] = text_array;
    std::string return_str = notify.toStyledString();
    session->send(return_str, MSG_NOTIFY_TEXT_CHAT_MSG_REQ);
    return Status::OK;
}
