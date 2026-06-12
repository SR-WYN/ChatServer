#pragma once
#include "ChatServiceClient.h"
#include "Singleton.h"
#include "data.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <grpcpp/grpcpp.h>
#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>
#include <queue>
#include <unordered_map>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using message::AddFriendReq;
using message::AddFriendRsp;

using message::AuthFriendReq;
using message::AuthFriendRsp;

using message::ChatService;

using message::TextChatData;
using message::TextChatMsgReq;
using message::TextChatMsgRsp;

class ChatConPool;

class ChatGrpcClient : public Singleton<ChatGrpcClient>, public ChatServiceClient
{
    friend class Singleton<ChatGrpcClient>;

public:
    ~ChatGrpcClient() override;
    AddFriendRsp NotifyAddFriend(const std::string &rpc_host, const std::string &rpc_port,
                                 const AddFriendReq &req) override;
    AuthFriendRsp NotifyAuthFriend(const std::string &rpc_host, const std::string &rpc_port,
                                   const AuthFriendReq &req) override;
    TextChatMsgRsp NotifyTextChatMsg(const std::string &rpc_host, const std::string &rpc_port,
                                     const TextChatMsgReq &req,
                                     const Json::Value &root_value) override;

private:
    ChatGrpcClient();
    ChatConPool &getOrCreatePool(const std::string &host, const std::string &port);
    std::unordered_map<std::string, std::unique_ptr<ChatConPool>> _pools;
};
