// ChatGrpcServiceImpl.cpp
#include "ChatGrpcServiceImpl.h"
#include "CSession.h"
#include "Log.h"
#include "LogModule.h"
#include "UserSessionManager.h"
#include "const.h"
#include "message.pb.h"
#include "defer.h"
#include <chrono>
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
    const auto start = std::chrono::steady_clock::now();
    auto touid = request->touid();
    auto session = _session_manager->getSession(touid);

    utils::Defer defer([request, reply]() {
        reply->set_error(ErrorCodes::SUCCESS);
        reply->set_applyuid(request->applyuid());
        reply->set_touid(request->touid());
    });

    LOGI(LogModule::Grpc, "NotifyAddFriend applyuid={} touid={} online={}", request->applyuid(),
         touid, session != nullptr);

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

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    LOGI(LogModule::Grpc, "NotifyAddFriend delivered applyuid={} touid={} cost={}ms",
         request->applyuid(), touid, cost_ms);
    return Status::OK;
}

Status ChatGrpcServiceImpl::NotifyAuthFriend(ServerContext* context, const AuthFriendReq* request,
                                             AuthFriendRsp* reply)
{
    (void)context;
    const auto start = std::chrono::steady_clock::now();
    auto touid = request->touid();
    auto fromuid = request->fromuid();
    auto session = _session_manager->getSession(touid);

    utils::Defer defer([request, reply]() {
        reply->set_error(ErrorCodes::SUCCESS);
        reply->set_fromuid(request->fromuid());
        reply->set_touid(request->touid());
    });

    LOGI(LogModule::Grpc, "NotifyAuthFriend fromuid={} touid={} online={}", fromuid, touid,
         session != nullptr);

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

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    LOGI(LogModule::Grpc, "NotifyAuthFriend delivered fromuid={} touid={} error={} cost={}ms",
         fromuid, touid, return_value["error"].asInt(), cost_ms);
    return Status::OK;
}

Status ChatGrpcServiceImpl::NotifyTextChatMsg(ServerContext* context,
                                              const TextChatMsgReq* request,
                                              TextChatMsgRsp* reply)
{
    (void)context;
    const auto start = std::chrono::steady_clock::now();
    auto touid = request->touid();
    auto session = _session_manager->getSession(touid);
    reply->set_error(ErrorCodes::SUCCESS);
    reply->set_recipient_online(session != nullptr);

    LOGI(LogModule::Grpc,
         "NotifyTextChatMsg fromuid={} touid={} msg_count={} local_online={}",
         request->fromuid(), touid, request->textmsgs_size(), session != nullptr);

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

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    LOGI(LogModule::Grpc, "NotifyTextChatMsg delivered fromuid={} touid={} cost={}ms",
         request->fromuid(), touid, cost_ms);
    return Status::OK;
}

Status ChatGrpcServiceImpl::NotifyImageChatMsg(ServerContext* context,
                                               const ImageChatMsgReq* request,
                                               ImageChatMsgRsp* reply)
{
    (void)context;
    const auto start = std::chrono::steady_clock::now();
    auto touid = request->touid();
    auto session = _session_manager->getSession(touid);
    reply->set_error(ErrorCodes::SUCCESS);

    LOGI(LogModule::Grpc, "NotifyImageChatMsg fromuid={} touid={} image_count={} local_online={}",
         request->fromuid(), touid, request->imagemsgs_size(), session != nullptr);

    if (session == nullptr)
    {
        return Status::OK;
    }

    Json::Value notify;
    notify["error"] = ErrorCodes::SUCCESS;
    notify["fromuid"] = request->fromuid();
    notify["touid"] = request->touid();

    Json::Value image_array;
    for (const auto& img_data : request->imagemsgs())
    {
        Json::Value element;
        element["msgid"] = img_data.msgid();
        element["url"] = img_data.url();
        element["width"] = img_data.width();
        element["height"] = img_data.height();
        element["size"] = static_cast<Json::Int64>(img_data.size());
        element["filename"] = img_data.filename();
        image_array.append(element);
    }
    notify["image_array"] = image_array;
    std::string return_str = notify.toStyledString();
    session->send(return_str, MSG_NOTIFY_IMAGE_CHAT_MSG_REQ);

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    LOGI(LogModule::Grpc, "NotifyImageChatMsg delivered fromuid={} touid={} cost={}ms",
         request->fromuid(), touid, cost_ms);
    return Status::OK;
}

Status ChatGrpcServiceImpl::KickUser(ServerContext* context, const KickUserReq* request,
                                     KickUserRsp* reply)
{
    (void)context;
    const auto start = std::chrono::steady_clock::now();
    int uid = request->uid();
    reply->set_error(ErrorCodes::SUCCESS);

    LOGI(LogModule::Grpc, "KickUser uid={} reason={}", uid, request->reason());

    if (uid <= 0)
    {
        LOGW(LogModule::Grpc, "KickUser: invalid uid={}", uid);
        return Status::OK;
    }

    auto session = _session_manager->getSession(uid);
    if (session == nullptr)
    {
        LOGI(LogModule::Grpc, "KickUser: user not online uid={}", uid);
        return Status::OK;
    }

    // 发送踢人通知后关闭连接
    session->closeAfterSend();

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    LOGI(LogModule::Grpc, "KickUser delivered uid={} cost={}ms", uid, cost_ms);
    return Status::OK;
}
