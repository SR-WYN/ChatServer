#pragma once

#include "ChatRuntimeConfig.h"
#include <optional>

class ChatNodeSlotSelector
{
public:
    /** 占用首个可 bind 的端口槽位 */
    static std::optional<SelfNodeProfile> acquire();
    /** 占槽并成功向 Status 注册；注册名冲突或 Status 不可达时尝试下一槽位 */
    static std::optional<SelfNodeProfile> acquireAndRegister();
};
