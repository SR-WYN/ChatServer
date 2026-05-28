#pragma once
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
using grpc::Status;
using grpc::ClientContext;

using message::AddFriendReq;
using message::AddFriendRsp;

using message::AuthFriendReq;
using message::AuthFriendRsp;

using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;
using message::ChatService;;

using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::TextChatData;

class ChatConPool;

class ChatGrpcClient : public Singleton<ChatGrpcClient>
{
    friend class Singleton<ChatGrpcClient>;

public:
    ~ChatGrpcClient() override;
    AddFriendRsp NotifyAddFriend(const std::string &rpc_host, const std::string &rpc_port,
                               const AddFriendReq &req);
    AuthFriendRsp NotifyAuthFriend(const std::string &rpc_host, const std::string &rpc_port,
                                   const AuthFriendReq &req);
    bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo> &user_info);
    TextChatMsgRsp NotifyTextChatMsg(const std::string &rpc_host, const std::string &rpc_port,
                                     const TextChatMsgReq &req, const Json::Value &root_value);

private:
    ChatGrpcClient();
    ChatConPool &getOrCreatePool(const std::string &host, const std::string &port);
    std::unordered_map<std::string, std::unique_ptr<ChatConPool>> _pools;
};