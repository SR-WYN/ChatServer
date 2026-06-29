// business_constants.h - 业务配置常量
#pragma once
#include <cstddef>

namespace constants::business
{

inline constexpr int kTokenTtlSeconds = 300;                 // 登录 token TTL，与 StatusServer 一致
inline constexpr int kTokenRefreshThresholdSeconds = 100;    // TTL 剩余不足 1/3 时刷新
inline constexpr int kRouteCacheDefaultTtlSeconds = 60;      // 跨服用户路由缓存默认 TTL
inline constexpr std::size_t kUserInfoCacheCapacity = 10000; // 用户信息缓存容量

} // namespace constants::business