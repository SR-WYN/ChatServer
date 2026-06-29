// StatusGrpcClientImpl.cpp
#include "StatusGrpcClientImpl.h"
#include "ConfigMgr.h"
#include "Log.h"
#include "LogModule.h"
#include "StatusConPool.h"
#include "const.h"
#include "utils.h"
#include "message.pb.h"
#include <chrono>
#include <grpcpp/grpcpp.h>

using message::BindUserToNodeReq;
using message::BindUserToNodeRsp;
using message::DeleteFileTokenReq;
using message::DeleteFileTokenRsp;
using message::GetFileServerReq;
using message::GetFileServerRsp;
using message::GetUserNodeReq;
using message::GetUserNodeRsp;
using message::HeartbeatNodeReq;
using message::HeartbeatNodeRsp;
using message::RefreshTokenTTLReq;
using message::RefreshTokenTTLRsp;
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
    LOGI(LogModule::Grpc, "StatusGrpcClient connecting to StatusServer at {}:{}", host, port);
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
        LOGE(LogModule::Grpc, "registerNode: failed to get connection");
        return false;
    }
    Status status = stub->RegisterNode(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });
    if (!status.ok())
    {
        LOGE(LogModule::Grpc,
             "registerNode RPC failed name={} instance={} code={} msg={}", node.name,
             node.instance_uid, static_cast<int>(status.error_code()), status.error_message());
        return false;
    }
    if (reply.error() != ErrorCodes::SUCCESS)
    {
        LOGW(LogModule::Grpc, "registerNode failed name={} instance={} err={}", node.name,
             node.instance_uid, reply.error());
        return false;
    }
    LOGI(LogModule::Grpc, "registerNode success name={} instance={}", node.name, node.instance_uid);
    return true;
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
        LOGE(LogModule::Grpc, "unregisterNode: failed to get connection");
        return false;
    }
    Status status = stub->UnregisterNode(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });
    if (!status.ok())
    {
        LOGE(LogModule::Grpc,
             "unregisterNode RPC failed name={} instance={} code={} msg={}", node.name,
             node.instance_uid, static_cast<int>(status.error_code()), status.error_message());
        return false;
    }
    LOGI(LogModule::Grpc, "unregisterNode success name={} instance={} err={}", node.name,
         node.instance_uid, reply.error());
    return status.ok() && reply.error() == ErrorCodes::SUCCESS;
}

bool StatusGrpcClientImpl::heartbeatNode(const std::string& name, const std::string& instance_id)
{
    ClientContext context;
    HeartbeatNodeReq request;
    HeartbeatNodeRsp reply;
    request.set_name(name);
    request.set_instance_id(instance_id);
    auto stub = _pool->getConnection();
    if (!stub)
    {
        LOGE(LogModule::Grpc, "heartbeatNode: failed to get connection");
        return false;
    }
    Status status = stub->HeartbeatNode(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });
    if (!status.ok())
    {
        LOGW(LogModule::Grpc,
             "heartbeatNode RPC failed name={} instance={} code={} msg={}", name, instance_id,
             static_cast<int>(status.error_code()), status.error_message());
        return false;
    }
    if (reply.error() != ErrorCodes::SUCCESS)
    {
        LOGW(LogModule::Grpc, "heartbeatNode failed name={} instance={} err={}", name, instance_id,
             reply.error());
        return false;
    }
    return true;
}

std::optional<UserNodeLocation> StatusGrpcClientImpl::getUserNode(int uid)
{
    const auto start = std::chrono::steady_clock::now();
    ClientContext context;
    GetUserNodeReq request;
    GetUserNodeRsp reply;
    request.set_uid(uid);
    auto stub = _pool->getConnection();
    if (!stub)
    {
        LOGE(LogModule::Grpc, "getUserNode: failed to get connection uid={}", uid);
        return std::nullopt;
    }
    Status status = stub->GetUserNode(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });
    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    if (!status.ok())
    {
        LOGE(LogModule::Grpc,
             "getUserNode RPC failed uid={} code={} msg={} cost={}ms", uid,
             static_cast<int>(status.error_code()), status.error_message(), cost_ms);
        return std::nullopt;
    }
    if (reply.error() != ErrorCodes::SUCCESS)
    {
        LOGW(LogModule::Grpc, "getUserNode failed uid={} err={} cost={}ms", uid, reply.error(),
             cost_ms);
        return std::nullopt;
    }
    UserNodeLocation loc;
    loc.node_name = reply.node_name();
    loc.rpc_host = reply.rpc_host();
    loc.rpc_port = reply.rpc_port();
    LOGI(LogModule::Grpc, "getUserNode success uid={} node={} cost={}ms", uid, loc.node_name,
         cost_ms);
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
        LOGE(LogModule::Grpc, "bindUserToNode: failed to get connection uid={}", uid);
        return false;
    }
    Status status = stub->BindUserToNode(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });
    if (!status.ok())
    {
        LOGE(LogModule::Grpc,
             "bindUserToNode RPC failed uid={} node={} code={} msg={}", uid, node_name,
             static_cast<int>(status.error_code()), status.error_message());
        return false;
    }
    if (reply.error() != ErrorCodes::SUCCESS)
    {
        LOGW(LogModule::Grpc, "bindUserToNode failed uid={} node={} err={}", uid, node_name,
             reply.error());
        return false;
    }
    LOGI(LogModule::Grpc, "bindUserToNode success uid={} node={}", uid, node_name);
    return true;
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
        LOGE(LogModule::Grpc, "unbindUser: failed to get connection uid={}", uid);
        return false;
    }
    Status status = stub->UnbindUser(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });
    if (!status.ok())
    {
        LOGE(LogModule::Grpc, "unbindUser RPC failed uid={} code={} msg={}", uid,
             static_cast<int>(status.error_code()), status.error_message());
        return false;
    }
    LOGI(LogModule::Grpc, "unbindUser success uid={} err={}", uid, reply.error());
    return status.ok() && reply.error() == ErrorCodes::SUCCESS;
}

int StatusGrpcClientImpl::validateToken(int uid, const std::string& token)
{
    const auto start = std::chrono::steady_clock::now();
    ClientContext context;
    ValidateTokenReq request;
    ValidateTokenRsp reply;
    request.set_uid(uid);
    request.set_token(token);

    auto stub = _pool->getConnection();
    if (!stub)
    {
        LOGE(LogModule::Grpc, "validateToken: failed to get connection uid={}", uid);
        return ErrorCodes::RPC_FAILED;
    }

    Status status = stub->ValidateToken(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();

    if (!status.ok())
    {
        LOGE(LogModule::Grpc,
             "validateToken RPC failed uid={} code={} msg={} cost={}ms", uid,
             static_cast<int>(status.error_code()), status.error_message(), cost_ms);
        return ErrorCodes::RPC_FAILED;
    }

    LOGI(LogModule::Grpc, "validateToken uid={} token={} result={} cost={}ms", uid, token,
         reply.error(), cost_ms);
    return reply.error();
}

bool StatusGrpcClientImpl::refreshTokenTTL(int uid)
{
    const auto start = std::chrono::steady_clock::now();
    ClientContext context;
    RefreshTokenTTLReq request;
    RefreshTokenTTLRsp reply;
    request.set_uid(uid);

    auto stub = _pool->getConnection();
    if (!stub)
    {
        LOGE(LogModule::Grpc, "refreshTokenTTL: failed to get connection uid={}", uid);
        return false;
    }

    Status status = stub->RefreshTokenTTL(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();

    if (!status.ok())
    {
        LOGE(LogModule::Grpc,
             "refreshTokenTTL RPC failed uid={} code={} msg={} cost={}ms", uid,
             static_cast<int>(status.error_code()), status.error_message(), cost_ms);
        return false;
    }

    if (reply.error() != ErrorCodes::SUCCESS)
    {
        LOGW(LogModule::Grpc, "refreshTokenTTL failed uid={} err={} cost={}ms", uid,
             reply.error(), cost_ms);
        return false;
    }

    LOGI(LogModule::Grpc, "refreshTokenTTL success uid={} cost={}ms", uid, cost_ms);
    return true;
}

std::optional<FileServerInfo> StatusGrpcClientImpl::getFileServer(int uid)
{
    const auto start = std::chrono::steady_clock::now();
    ClientContext context;
    GetFileServerReq request;
    GetFileServerRsp reply;
    request.set_uid(uid);
    auto stub = _pool->getConnection();
    if (!stub)
    {
        LOGE(LogModule::Grpc, "getFileServer: failed to get connection uid={}", uid);
        return std::nullopt;
    }
    Status status = stub->GetFileServer(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });
    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    if (!status.ok())
    {
        LOGE(LogModule::Grpc,
             "getFileServer RPC failed uid={} code={} msg={} cost={}ms", uid,
             static_cast<int>(status.error_code()), status.error_message(), cost_ms);
        return std::nullopt;
    }
    if (reply.error() != ErrorCodes::SUCCESS)
    {
        LOGW(LogModule::Grpc, "getFileServer failed uid={} err={} cost={}ms", uid, reply.error(),
             cost_ms);
        return std::nullopt;
    }
    FileServerInfo info;
    info.host = reply.host();
    info.port = reply.port();
    info.token = reply.token();
    LOGI(LogModule::Grpc, "getFileServer success uid={} host={} port={} cost={}ms", uid, info.host,
         info.port, cost_ms);
    return info;
}

bool StatusGrpcClientImpl::deleteFileToken(int uid)
{
    ClientContext context;
    DeleteFileTokenReq request;
    DeleteFileTokenRsp reply;
    request.set_uid(uid);
    auto stub = _pool->getConnection();
    if (!stub)
    {
        LOGE(LogModule::Grpc, "deleteFileToken: failed to get connection uid={}", uid);
        return false;
    }
    Status status = stub->DeleteFileToken(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });
    if (!status.ok())
    {
        LOGE(LogModule::Grpc, "deleteFileToken RPC failed uid={} code={} msg={}", uid,
             static_cast<int>(status.error_code()), status.error_message());
        return false;
    }
    LOGI(LogModule::Grpc, "deleteFileToken success uid={} err={}", uid, reply.error());
    return status.ok() && reply.error() == ErrorCodes::SUCCESS;
}
