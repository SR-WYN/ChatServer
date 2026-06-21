#include "UserNodeRouteCacheImpl.h"

std::optional<UserNodeLocation> UserNodeRouteCacheImpl::get(int uid)
{
    auto opt = _cache.get(uid);
    if (!opt)
    {
        return std::nullopt;
    }
    if (std::chrono::steady_clock::now() > opt->expire_time)
    {
        _cache.erase(uid);
        return std::nullopt;
    }
    return opt->location;
}

void UserNodeRouteCacheImpl::put(int uid, const UserNodeLocation& loc, int ttl_seconds)
{
    RouteEntry entry;
    entry.location = loc;
    entry.expire_time = std::chrono::steady_clock::now() + std::chrono::seconds(ttl_seconds);
    _cache.put(uid, entry);
}

void UserNodeRouteCacheImpl::invalidate(int uid)
{
    _cache.erase(uid);
}
