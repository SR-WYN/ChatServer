// UserSearchImpl.cpp
#include "UserSearchImpl.h"
#include "CSession.h"
#include "Log.h"
#include "LogModule.h"
#include "UserInfoCache.h"
#include "const.h"
#include "defer.h"
#include <chrono>
#include <json/reader.h>
#include <json/value.h>
#include <memory>
#include <string>

UserSearchImpl::UserSearchImpl(std::shared_ptr<UserInfoCache> user_info_cache)
    : _user_info_cache(std::move(user_info_cache))
{
}

void UserSearchImpl::handle(std::shared_ptr<CSession> session, const std::string& msg_data)
{
    const auto start = std::chrono::steady_clock::now();
    Json::Reader reader;
    Json::Value root;
    Json::Value result;
    utils::Defer defer([&result, session]() {
        std::string return_str = result.toStyledString();
        session->send(return_str, MSG_SEARCH_USER_RSP);
    });

    if (!reader.parse(msg_data, root) || !root.isMember("uid"))
    {
        LOGW(LogModule::User, "search user: invalid JSON session={}", session->getSessionId());
        result["error"] = ErrorCodes::ERROR_JSON;
        return;
    }

    std::string key;
    if (root["uid"].isInt())
    {
        key = std::to_string(root["uid"].asInt());
        LOGI(LogModule::User, "search user by uid session={} key={}", session->getSessionId(), key);
        _user_info_cache->fillSearchResultByUid(key, result);
    }
    else
    {
        key = root["uid"].asString();
        LOGI(LogModule::User, "search user by name session={} key={}", session->getSessionId(),
             key);
        _user_info_cache->fillSearchResultByName(key, result);
    }

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    LOGI(LogModule::User, "search user result session={} key={} error={} cost={}ms",
         session->getSessionId(), key, result["error"].asInt(), cost_ms);
}
