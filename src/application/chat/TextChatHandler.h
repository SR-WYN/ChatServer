#pragma once

#include <memory>
#include <string>

class CSession;

class TextChatHandler {
public:
    static void handleTextChat(std::shared_ptr<CSession> session, const short &msg_id,
                               const std::string &msg_data);

private:
    TextChatHandler() = delete;
};
