// ChatGrpcClientImpl.cpp
#include "ChatGrpcClientImpl.h"
#include "const.h"
#include "message.pb.h"
#include "utils.h"
#include <grpcpp/client_context.h>
#include <mutex>
#include <sstream>

namespace
{
std::string endpointKey(const std::string& host, const std::string& port)
{
    return host + ":" + port;
}
} // namespace

ChatGrpcClientImpl::ChatGrpcClientImpl() = default;

ChatGrpcClientImpl::~ChatGrpcClientImpl() = default;

ChatConPool& ChatGrpcClientImpl::getOrCreatePool(const std::string& host,
                                                 const std::string& port)
{
    const std::string key = endpointKey(host, port);
    auto find_iter = _pools.find(key);
    if (find_iter != _pools.end())
    {
        return *find_iter->second;
    }
    auto pool = std::make_unique<ChatConPool>(5, host, port);
    auto& ref = *pool;
    _pools.emplace(key, std::move(pool));
    return ref;
}

AddFriendRsp ChatGrpcClientImpl::NotifyAddFriend(const std::string& rpc_host,
                                                 const std::string& rpc_port,
                                                 const AddFriendReq& req)
{
    AddFriendRsp rsp;
    utils::Defer defer([&rsp, &req]() {
        rsp.set_error(ErrorCodes::SUCCESS);
        rsp.set_applyuid(req.applyuid());
        rsp.set_touid(req.touid());
    });
    if (rpc_host.empty() || rpc_port.empty())
    {
        rsp.set_error(ErrorCodes::RPC_FAILED);
        return rsp;
    }
    auto& pool = getOrCreatePool(rpc_host, rpc_port);
    grpc::ClientContext context;
    auto stub = pool.getConnection();
    grpc::Status status = stub->NotifyAddFriend(&context, req, &rsp);
    utils::Defer defercon([&stub, &pool]() {
        pool.returnConnection(std::move(stub));
    });
    if (!status.ok())
    {
        rsp.set_error(ErrorCodes::RPC_FAILED);
    }
    return rsp;
}

ImageChatMsgRsp ChatGrpcClientImpl::NotifyImageChatMsg(const std::string& rpc_host,
                                                       const std::string& rpc_port,
                                                       const ImageChatMsgReq& req,
                                                       const Json::Value& root_value)
{
    (void)root_value;
    ImageChatMsgRsp rsp;
    rsp.set_error(ErrorCodes::SUCCESS);
    utils::Defer defer([&rsp, &req]() {
        rsp.set_fromuid(req.fromuid());
        rsp.set_touid(req.touid());
    });
    if (rpc_host.empty() || rpc_port.empty())
    {
        rsp.set_error(ErrorCodes::RPC_FAILED);
        return rsp;
    }
    auto& pool = getOrCreatePool(rpc_host, rpc_port);
    grpc::ClientContext context;
    auto stub = pool.getConnection();
    grpc::Status status = stub->NotifyImageChatMsg(&context, req, &rsp);
    utils::Defer defercon([&stub, &pool]() {
        pool.returnConnection(std::move(stub));
    });
    if (!status.ok())
    {
        rsp.set_error(ErrorCodes::RPC_FAILED);
    }
    return rsp;
}

AuthFriendRsp ChatGrpcClientImpl::NotifyAuthFriend(const std::string& rpc_host,
                                                   const std::string& rpc_port,
                                                   const AuthFriendReq& req)
{
    AuthFriendRsp rsp;
    rsp.set_error(ErrorCodes::SUCCESS);
    utils::Defer defer([&rsp, &req]() {
        rsp.set_fromuid(req.fromuid());
        rsp.set_touid(req.touid());
    });
    if (rpc_host.empty() || rpc_port.empty())
    {
        rsp.set_error(ErrorCodes::RPC_FAILED);
        return rsp;
    }
    auto& pool = getOrCreatePool(rpc_host, rpc_port);
    grpc::ClientContext context;
    auto stub = pool.getConnection();
    grpc::Status status = stub->NotifyAuthFriend(&context, req, &rsp);
    utils::Defer defercon([&stub, &pool]() {
        pool.returnConnection(std::move(stub));
    });
    if (!status.ok())
    {
        rsp.set_error(ErrorCodes::RPC_FAILED);
    }
    return rsp;
}

TextChatMsgRsp ChatGrpcClientImpl::NotifyTextChatMsg(const std::string& rpc_host,
                                                     const std::string& rpc_port,
                                                     const TextChatMsgReq& req,
                                                     const Json::Value& root_value)
{
    TextChatMsgRsp rsp;
    rsp.set_error(ErrorCodes::SUCCESS);
    utils::Defer defer([&rsp, &req, &root_value]() {
        rsp.set_fromuid(req.fromuid());
        rsp.set_touid(req.touid());
        for (const auto& text_data : req.textmsgs())
        {
            message::TextChatData* new_msg = rsp.add_textmsgs();
            new_msg->set_msgid(text_data.msgid());
            new_msg->set_msgcontent(text_data.msgcontent());
        }
    });
    if (rpc_host.empty() || rpc_port.empty())
    {
        rsp.set_error(ErrorCodes::RPC_FAILED);
        return rsp;
    }
    auto& pool = getOrCreatePool(rpc_host, rpc_port);
    grpc::ClientContext context;
    auto stub = pool.getConnection();
    grpc::Status status = stub->NotifyTextChatMsg(&context, req, &rsp);
    utils::Defer defercon([&stub, &pool]() {
        pool.returnConnection(std::move(stub));
    });
    if (!status.ok())
    {
        rsp.set_error(ErrorCodes::RPC_FAILED);
    }
    return rsp;
}
