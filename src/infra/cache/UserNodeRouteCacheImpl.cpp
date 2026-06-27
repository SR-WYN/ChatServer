// UserNodeRouteCacheImpl.cpp
#include "UserNodeRouteCacheImpl.h"
#include "Log.h"
#include "LogModule.h"

std::optional<UserNodeLocation> UserNodeRouteCacheImpl::get(int uid)
{
    auto opt = _cache.get(uid);
    if (!opt)
    {
        LOGD(LogModule::Net, "route cache miss uid={}", uid);
        return std::nullopt;
    }
    if (std::chrono::steady_clock::now() > opt->expire_time)
    {
        _cache.erase(uid);
        LOGD(LogModule::Net, "route cache expired uid={}", uid);
        return std::nullopt;
    }
    LOGD(LogModule::Net, "route cache hit uid={} node={}", uid, opt->location.node_name);
    return opt->location;
}

void UserNodeRouteCacheImpl::put(int uid, const UserNodeLocation& loc, int ttl_seconds)
{
    RouteEntry entry;
    entry.location = loc;
    entry.expire_time = std::chrono::steady_clock::now() + std::chrono::seconds(ttl_seconds);
    _cache.put(uid, entry);
    LOGI(LogModule::Net, "route cache put uid={} node={} ttl={}s", uid, loc.node_name, ttl_seconds);
}

void UserNodeRouteCacheImpl::invalidate(int uid)
{
    _cache.erase(uid);
    LOGI(LogModule::Net, "route cache invalidated uid={}", uid);
}
