// FileTransferImpl.cpp - 文件传输业务实现
#include "FileTransferImpl.h"
#include "CSession.h"
#include "Log.h"
#include "StatusGrpcClient.h"
#include "const.h"
#include "utils.h"
#include <json/reader.h>
#include <json/value.h>
#include <json/writer.h>

FileTransferImpl::FileTransferImpl(std::shared_ptr<StatusGrpcClient> status_client)
    : _status_client(std::move(status_client))
{
}

void FileTransferImpl::handleGetFileServer(std::shared_ptr<CSession> session,
                                           const std::string& msg_data)
{
    Json::Reader reader;
    Json::Value root;
    Json::Value rsp;
    rsp["error"] = ErrorCodes::SUCCESS;

    utils::Defer defer([&rsp, session]() {
        std::string return_str = rsp.toStyledString();
        session->send(return_str, MSG_FILE_TRANSFER_RSP);
    });

    if (!reader.parse(msg_data, root) || root.isNull() || root.empty())
    {
        rsp["error"] = ErrorCodes::ERROR_JSON;
        return;
    }

    const int uid = root.isMember("uid") ? root["uid"].asInt() : session->getUserId();
    if (uid <= 0)
    {
        rsp["error"] = ErrorCodes::TOKEN_INVALID;
        return;
    }

    auto file_server = _status_client->getFileServer(uid);
    if (!file_server)
    {
        Log::warn(LogModule::Chat, "handleGetFileServer: no available file server for uid={}",
                  uid);
        rsp["error"] = ErrorCodes::RPC_FAILED;
        return;
    }

    rsp["host"] = file_server->host;
    rsp["port"] = file_server->port;
    rsp["token"] = file_server->token;
    Log::info(LogModule::Chat, "handleGetFileServer: uid={} host={} port={}", uid,
              file_server->host, file_server->port);
}

void FileTransferImpl::handleFileTransferDone(std::shared_ptr<CSession> session,
                                              const std::string& msg_data)
{
    (void)session;
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root) || root.isNull() || root.empty())
    {
        return;
    }

    const int uid = root.isMember("uid") ? root["uid"].asInt() : session->getUserId();
    if (uid <= 0)
    {
        return;
    }

    _status_client->deleteFileToken(uid);
    Log::info(LogModule::Chat, "handleFileTransferDone: token deleted for uid={}", uid);
}
