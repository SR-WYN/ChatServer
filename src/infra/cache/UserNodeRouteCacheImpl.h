// UserNodeRouteCacheImpl.h - 跨服用户路由缓存实现
// 本地 LRU 缓存 + TTL 过期，减少对 StatusServer 的 RPC 调用
#pragma once

#include "LRUCache.h"
#include "UserNodeRouteCache.h"
#include <chrono>

struct RouteEntry
{
    UserNodeLocation location;
    std::chrono::steady_clock::time_point expire_time;
};

/// 跨服用户路由缓存实现 —— 本地 LRU 缓存 + TTL 过期
class UserNodeRouteCacheImpl : public UserNodeRouteCache
{
public:
    UserNodeRouteCacheImpl() = default;
    ~UserNodeRouteCacheImpl() override = default;

    std::optional<UserNodeLocation> get(int uid) override;
    void put(int uid, const UserNodeLocation& loc, int ttl_seconds = DEFAULT_TTL) override;
    void invalidate(int uid) override;

private:
    // uid -> RouteEntry 路由缓存（LRU，默认容量 10000）
    LRUCache<int, RouteEntry> _cache;

    static constexpr int DEFAULT_TTL = 60;          // 默认 60 秒过期
};
