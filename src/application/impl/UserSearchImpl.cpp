// UserSearchImpl.cpp
#include "UserSearchImpl.h"
#include "CSession.h"
#include "UserInfoCache.h"
#include "const.h"
#include "utils.h"
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
    Json::Reader reader;
    Json::Value root;
    Json::Value result;
    utils::Defer defer([&result, session]() {
        std::string return_str = result.toStyledString();
        session->send(return_str, MSG_SEARCH_USER_RSP);
    });

    if (!reader.parse(msg_data, root) || !root.isMember("uid"))
    {
        result["error"] = ErrorCodes::ERROR_JSON;
        return;
    }

    if (root["uid"].isInt())
    {
        _user_info_cache->fillSearchResultByUid(std::to_string(root["uid"].asInt()), result);
    }
    else
    {
        _user_info_cache->fillSearchResultByName(root["uid"].asString(), result);
    }
}
