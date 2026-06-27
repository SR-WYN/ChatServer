// FileTransfer.h - 文件传输业务接口
// 处理客户端获取 FileServer 地址及上传完成后的 token 清理请求。
#pragma once

#include <memory>
#include <string>

class CSession;

class FileTransfer
{
public:
    virtual ~FileTransfer() = default;

    /// 处理客户端获取 FileServer 地址的请求
    virtual void handleGetFileServer(std::shared_ptr<CSession> session,
                                     const std::string& msg_data) = 0;

    /// 处理客户端上传完成后的 token 清理通知
    virtual void handleFileTransferDone(std::shared_ptr<CSession> session,
                                        const std::string& msg_data) = 0;
};
