#pragma once 

#include "data.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <memory>

using message::ChatService;
using message::AddFriendReq;
using message::AddFriendRsp;
using message::AuthFriendReq;
using message::AuthFriendRsp;
using message::TextChatMsgReq;
using message::TextChatMsgRsp;

using grpc::ServerContext;
using grpc::Status;

class ChatServiceImpl final : public ChatService::Service
{
public:
    ChatServiceImpl();
    virtual Status NotifyAddFriend(ServerContext* context,const AddFriendReq* request,AddFriendRsp* reply) override;
    virtual Status NotifyAuthFriend(ServerContext* context,const AuthFriendReq* request,AuthFriendRsp* reply) override;
    virtual Status NotifyTextChatMsg(ServerContext* context,const TextChatMsgReq* request,TextChatMsgRsp* reply) override;
    bool getBaseInfo(std::string base_key,int uid,std::shared_ptr<UserInfo>& user_info);
private:
};