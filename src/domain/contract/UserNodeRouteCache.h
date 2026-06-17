// UserNodeRouteCache.h - 跨服用户路由缓存接口
// 缓存 uid → UserChatLocation 映射，减少对 StatusServer 的 RPC 调用
#pragma once

#include "StatusServiceClient.h"
#include <chrono>
#include <optional>

/// 跨服用户路由缓存接口
class UserNodeRouteCache
{
public:
    virtual ~UserNodeRouteCache() = default;

    /// 查询缓存，命中且未过期则返回
    virtual std::optional<UserChatLocation> get(int uid) = 0;

    /// 写入缓存
    /// @param uid 用户 ID
    /// @param loc 用户所在节点信息
    /// @param ttl_seconds 存活秒数，默认 60
    virtual void put(int uid, const UserChatLocation &loc, int ttl_seconds = 60) = 0;

    /// 使指定用户的缓存失效（用户下线 / RPC 失败时调用）
    virtual void invalidate(int uid) = 0;
};
