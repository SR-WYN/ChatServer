// StatusGrpcClientImpl.cpp
#include "StatusGrpcClientImpl.h"
#include "ConfigMgr.h"
#include "StatusConPool.h"
#include "const.h"
#include "utils.h"
#include "message.pb.h"
#include <grpcpp/grpcpp.h>

using message::BindUserToNodeReq;
using message::BindUserToNodeRsp;
using message::GetUserNodeReq;
using message::GetUserNodeRsp;
using message::HeartbeatNodeReq;
using message::HeartbeatNodeRsp;
using message::RegisterNodeReq;
using message::RegisterNodeRsp;
using message::UnbindUserReq;
using message::UnbindUserRsp;
using message::UnregisterNodeReq;
using message::UnregisterNodeRsp;
using message::ValidateTokenReq;
using message::ValidateTokenRsp;

using grpc::ClientContext;
using grpc::Status;

StatusGrpcClientImpl::StatusGrpcClientImpl()
{
    auto& gCfgMgr = ConfigMgr::getInstance();
    std::string host = gCfgMgr["StatusServer"]["Host"];
    std::string port = gCfgMgr["StatusServer"]["Port"];
    _pool = std::make_unique<StatusConPool>(5, host, port);
}

StatusGrpcClientImpl::~StatusGrpcClientImpl() = default;

bool StatusGrpcClientImpl::registerNode(const NodeInfo& node)
{
    ClientContext context;
    RegisterNodeReq request;
    RegisterNodeRsp reply;
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
    Status status = stub->RegisterNode(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });
    return status.ok() && reply.error() == ErrorCodes::SUCCESS;
}

bool StatusGrpcClientImpl::unregisterNode(const NodeInfo& node)
{
    ClientContext context;
    UnregisterNodeReq request;
    UnregisterNodeRsp reply;
    request.set_name(node.name);
    request.set_instance_id(node.instance_uid);
    auto stub = _pool->getConnection();
    if (!stub)
    {
        return false;
    }
    Status status = stub->UnregisterNode(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });
    return status.ok() && reply.error() == ErrorCodes::SUCCESS;
}

bool StatusGrpcClientImpl::heartbeatNode(const std::string& name,
                                             const std::string& instance_id)
{
    ClientContext context;
    HeartbeatNodeReq request;
    HeartbeatNodeRsp reply;
    request.set_name(name);
    request.set_instance_id(instance_id);
    auto stub = _pool->getConnection();
    if (!stub)
    {
        return false;
    }
    Status status = stub->HeartbeatNode(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });
    return status.ok() && reply.error() == ErrorCodes::SUCCESS;
}

std::optional<UserNodeLocation> StatusGrpcClientImpl::getUserNode(int uid)
{
    ClientContext context;
    GetUserNodeReq request;
    GetUserNodeRsp reply;
    request.set_uid(uid);
    auto stub = _pool->getConnection();
    if (!stub)
    {
        return std::nullopt;
    }
    Status status = stub->GetUserNode(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });
    if (!status.ok() || reply.error() != ErrorCodes::SUCCESS)
    {
        return std::nullopt;
    }
    UserNodeLocation loc;
    loc.node_name = reply.node_name();
    loc.rpc_host = reply.rpc_host();
    loc.rpc_port = reply.rpc_port();
    return loc;
}

bool StatusGrpcClientImpl::bindUserToNode(int uid, const std::string& node_name)
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

bool StatusGrpcClientImpl::unbindUser(int uid)
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

int StatusGrpcClientImpl::validateToken(int uid, const std::string& token)
{
    ClientContext context;
    ValidateTokenReq request;
    ValidateTokenRsp reply;
    request.set_uid(uid);
    request.set_token(token);

    auto stub = _pool->getConnection();
    if (!stub)
    {
        return ErrorCodes::RPC_FAILED;
    }

    Status status = stub->ValidateToken(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });

    if (!status.ok())
    {
        return ErrorCodes::RPC_FAILED;
    }

    return reply.error();
}
