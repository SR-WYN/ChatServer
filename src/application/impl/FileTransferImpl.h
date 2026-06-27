// FileTransferImpl.h - 文件传输业务实现
#pragma once

#include "FileTransfer.h"
#include <memory>

class StatusGrpcClient;

class FileTransferImpl final : public FileTransfer
{
public:
    explicit FileTransferImpl(std::shared_ptr<StatusGrpcClient> status_client);

    void handleGetFileServer(std::shared_ptr<CSession> session,
                             const std::string& msg_data) override;
    void handleFileTransferDone(std::shared_ptr<CSession> session,
                                const std::string& msg_data) override;

private:
    std::shared_ptr<StatusGrpcClient> _status_client;
};
