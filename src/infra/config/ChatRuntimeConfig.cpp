#include "ChatRuntimeConfig.h"

ChatRuntimeConfig &ChatRuntimeConfig::getInstance()
{
    static ChatRuntimeConfig instance;
    return instance;
}

void ChatRuntimeConfig::setSelf(const SelfNodeProfile &node)
{
    _self = node;
    _has_self = true;
}

const SelfNodeProfile &ChatRuntimeConfig::self() const
{
    return _self;
}

bool ChatRuntimeConfig::hasSelf() const
{
    return _has_self;
}
