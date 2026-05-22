#include "UserHandler.h"
#include "CSession.h"
#include "UserCacheService.h"
#include "const.h"
#include "utils.h"
#include <json/reader.h>
#include <json/value.h>
#include <memory>
#include <string>

void UserHandler::handleSearchUser(std::shared_ptr<CSession> session, const short &msg_id,
                                   const std::string &msg_data)
{
    (void)msg_id;
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
    auto uid_str = root["uid"].asString();
    if (root["uid"].isInt())
    {
        UserCacheService::fillSearchResultByUid(std::to_string(root["uid"].asInt()), result);
    }
    else
    {
        UserCacheService::fillSearchResultByName(uid_str, result);
    }
}
