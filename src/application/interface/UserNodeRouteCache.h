// UserNodeRouteCache.h - 跨服用户路由缓存接口
#pragma once

#include "StatusGrpcClient.h"
#include <optional>

class UserNodeRouteCache
{
public:
    virtual ~UserNodeRouteCache() = default;

    virtual std::optional<UserNodeLocation> get(int uid) = 0;
    virtual void put(int uid, const UserNodeLocation& loc, int ttl_seconds = 60) = 0;
    virtual void invalidate(int uid) = 0;
};
