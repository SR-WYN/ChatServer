#pragma once

#include "CSession.h"
#include <memory>
#include <string>

class ChatHistoryHandler
{
public:
    static void handleHistory(std::shared_ptr<CSession> session, const short &msg_id,
                              const std::string &msg_data);
};
