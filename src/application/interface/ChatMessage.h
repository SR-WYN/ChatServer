// ChatMessage.h - 聊天消息业务接口
#pragma once

#include <memory>
#include <string>

class CSession;

class ChatMessage
{
public:
    virtual ~ChatMessage() = default;

    virtual void handleTextChat(std::shared_ptr<CSession> session,
                                const std::string& msg_data) = 0;
    virtual void handleHistory(std::shared_ptr<CSession> session,
                               const std::string& msg_data) = 0;
};
