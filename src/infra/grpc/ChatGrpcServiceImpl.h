// ChatGrpcServiceImpl.h - gRPC 入站适配器实现
#pragma once

#include "MySqlMgr.h"
#include "UserInfoCache.h"
#include "UserSessionManager.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <memory>

using message::AddFriendReq;
using message::AddFriendRsp;
using message::AuthFriendReq;
using message::AuthFriendRsp;
using message::ChatService;
using message::ImageChatMsgReq;
using message::ImageChatMsgRsp;
using message::TextChatMsgReq;
using message::TextChatMsgRsp;

using grpc::ServerContext;
using grpc::Status;

class ChatGrpcServiceImpl final : public ChatService::Service
{
public:
    ChatGrpcServiceImpl(std::shared_ptr<UserSessionManager> session_manager,
                        std::shared_ptr<UserInfoCache> user_info_cache,
                        std::shared_ptr<MySqlMgr> mysql_mgr);

    Status NotifyAddFriend(ServerContext* context, const AddFriendReq* request,
                           AddFriendRsp* reply) override;
    Status NotifyAuthFriend(ServerContext* context, const AuthFriendReq* request,
                            AuthFriendRsp* reply) override;
    Status NotifyTextChatMsg(ServerContext* context, const TextChatMsgReq* request,
                             TextChatMsgRsp* reply) override;
    Status NotifyImageChatMsg(ServerContext* context, const ImageChatMsgReq* request,
                              ImageChatMsgRsp* reply) override;

private:
    std::shared_ptr<UserSessionManager> _session_manager;
    std::shared_ptr<UserInfoCache> _user_info_cache;
    std::shared_ptr<MySqlMgr> _mysql_mgr;
};
