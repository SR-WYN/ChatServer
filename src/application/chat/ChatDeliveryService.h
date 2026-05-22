#pragma once

#include <json/value.h>

class ChatDeliveryService
{
public:
    static bool deliverTextChat(int from_uid, int to_uid, const Json::Value &text_array,
                                const Json::Value &return_value);
};
