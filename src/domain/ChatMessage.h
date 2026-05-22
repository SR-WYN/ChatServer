#pragma once

#include <cstdint>
#include <string>

struct ChatMessage
{
    uint64_t id = 0;
    std::string client_msg_id;
    int from_uid = 0;
    int to_uid = 0;
    std::string content;
};
