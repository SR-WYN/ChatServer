#include "StatusGrpcClient.h"
#include "ConfigMgr.h"
#include "StatusConPool.h"
#include "const.h"
#include "utils.h"

using message::BindUserToNodeReq;
using message::BindUserToNodeRsp;
using message::GetUserChatNodeReq;
using message::GetUserChatNodeRsp;
using message::HeartbeatChatNodeReq;
using message::HeartbeatChatNodeRsp;
using message::RegisterChatNodeReq;
using message::RegisterChatNodeRsp;
using message::UnbindUserReq;
using message::UnbindUserRsp;
using message::UnregisterChatNodeReq;
using message::UnregisterChatNodeRsp;

StatusGrpcClient::StatusGrpcClient()
{
    auto &gCfgMgr = ConfigMgr::getInstance();
    std::string host = gCfgMgr["StatusServer"]["Host"];
    std::string port = gCfgMgr["StatusServer"]["Port"];
    _pool.reset(new StatusConPool(5, host, port));
}

StatusGrpcClient::~StatusGrpcClient() = default;

GetChatServerRsp StatusGrpcClient::getChatServer(int uid)
{
    ClientContext context;
    GetChatServerRsp reply;
    GetChatServerReq request;
    request.set_uid(uid);
    auto stub = _pool->getConnection();
    Status status = stub->GetChatServer(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });
    if (status.ok())
    {
        return reply;
    }
    reply.set_error(ErrorCodes::RPC_FAILED);
    return reply;
}

bool StatusGrpcClient::registerChatNode(const NodeInfo &node)
{
    ClientContext context;
    RegisterChatNodeReq request;
    RegisterChatNodeRsp reply;
    request.set_name(node.name);
    request.set_client_host(node.client_advertise_host);
    request.set_client_port(node.client_advertise_port);
    request.set_rpc_host(node.rpc_advertise_host);
    request.set_rpc_port(node.rpc_advertise_port);
    request.set_instance_id(node.instance_uid);
    auto stub = _pool->getConnection();
    if (!stub)
    {
        return false;
    }
    Status status = stub->RegisterChatNode(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });
    return status.ok() && reply.error() == ErrorCodes::SUCCESS;
}

bool StatusGrpcClient::unregisterChatNode(const NodeInfo &node)
{
    ClientContext context;
    UnregisterChatNodeReq request;
    UnregisterChatNodeRsp reply;
    request.set_name(node.name);
    request.set_instance_id(node.instance_uid);
    auto stub = _pool->getConnection();
    if (!stub)
    {
        return false;
    }
    Status status = stub->UnregisterChatNode(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });
    return status.ok() && reply.error() == ErrorCodes::SUCCESS;
}

bool StatusGrpcClient::heartbeatChatNode(const std::string &name, const std::string &instance_id)
{
    ClientContext context;
    HeartbeatChatNodeReq request;
    HeartbeatChatNodeRsp reply;
    request.set_name(name);
    request.set_instance_id(instance_id);
    auto stub = _pool->getConnection();
    if (!stub)
    {
        return false;
    }
    Status status = stub->HeartbeatChatNode(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });
    return status.ok() && reply.error() == ErrorCodes::SUCCESS;
}

std::optional<UserChatLocation> StatusGrpcClient::getUserChatNode(int uid)
{
    ClientContext context;
    GetUserChatNodeReq request;
    GetUserChatNodeRsp reply;
    request.set_uid(uid);
    auto stub = _pool->getConnection();
    if (!stub)
    {
        return std::nullopt;
    }
    Status status = stub->GetUserChatNode(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });
    if (!status.ok() || reply.error() != ErrorCodes::SUCCESS)
    {
        return std::nullopt;
    }
    UserChatLocation loc;
    loc.node_name = reply.node_name();
    loc.rpc_host = reply.rpc_host();
    loc.rpc_port = reply.rpc_port();
    return loc;
}

bool StatusGrpcClient::bindUserToNode(int uid, const std::string &node_name)
{
    ClientContext context;
    BindUserToNodeReq request;
    BindUserToNodeRsp reply;
    request.set_uid(uid);
    request.set_node_name(node_name);
    auto stub = _pool->getConnection();
    if (!stub)
    {
        return false;
    }
    Status status = stub->BindUserToNode(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });
    return status.ok() && reply.error() == ErrorCodes::SUCCESS;
}

bool StatusGrpcClient::unbindUser(int uid)
{
    ClientContext context;
    UnbindUserReq request;
    UnbindUserRsp reply;
    request.set_uid(uid);
    auto stub = _pool->getConnection();
    if (!stub)
    {
        return false;
    }
    Status status = stub->UnbindUser(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });
    return status.ok() && reply.error() == ErrorCodes::SUCCESS;
}
