#include "StatusGrpcClient.h"
#include "ConfigMgr.h"
#include "StatusConPool.h"
#include "const.h"
#include "utils.h"

StatusGrpcClient::StatusGrpcClient()
{
    auto &gCfgMgr = ConfigMgr::getInstance();
    std::string host = gCfgMgr["StatusServer"]["Host"];
    std::string port = gCfgMgr["StatusServer"]["Port"];
    _pool.reset(new StatusConPool(5, host, port));
}

StatusGrpcClient::~StatusGrpcClient()
{
}

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
    else
    {
        reply.set_error(ErrorCodes::RPCFAILED);
        return reply;
    }
}

LoginRsp StatusGrpcClient::login(int uid, std::string token)
{
    ClientContext context;
    LoginRsp reply;
    LoginReq request;
    request.set_uid(uid);
    request.set_token(token);
    auto stub = _pool->getConnection();
    if (!stub)
    {
        reply.set_error(ErrorCodes::RPCFAILED);
        return reply;
    }
    Status status = stub->Login(&context, request, &reply);
    utils::Defer defer([&stub, this]() {
        _pool->returnConnection(std::move(stub));
    });
    if (status.ok())
    {
        return reply;
    }
    else
    {
        reply.set_error(ErrorCodes::RPCFAILED);
        return reply;
    }
}