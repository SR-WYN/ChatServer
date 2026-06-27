// ChatMessageImpl.h - 聊天消息业务实现
#pragma once

#include "ChatMessage.h"
#include <json/value.h>
#include <memory>

class ChatGrpcClient;
class IdGenerator;
class MySqlMgr;
class RuntimeContext;
class StatusGrpcClient;
class UserNodeRouteCache;
class UserSessionManager;

class ChatMessageImpl final : public ChatMessage
{
public:
    ChatMessageImpl(std::shared_ptr<UserSessionManager> session_manager,
                    std::shared_ptr<UserNodeRouteCache> route_cache,
                    std::shared_ptr<MySqlMgr> mysql_mgr,
                    std::shared_ptr<StatusGrpcClient> status_client,
                    std::shared_ptr<ChatGrpcClient> chat_client,
                    std::shared_ptr<IdGenerator> id_generator,
                    const RuntimeContext& runtime_context);

    void handleTextChat(std::shared_ptr<CSession> session, const std::string& msg_data) override;
    void handleImageChat(std::shared_ptr<CSession> session, const std::string& msg_data) override;
    void handleHistory(std::shared_ptr<CSession> session, const std::string& msg_data) override;

private:
    bool deliverTextChat(int from_uid, int to_uid, const Json::Value& text_array,
                         const Json::Value& return_value);
    void persistOutgoingBatch(int from_uid, int to_uid, const Json::Value& text_array,
                              bool delivered_online);
    bool persistOneMessage(int from_uid, int to_uid, const std::string& msgid,
                           const std::string& content, bool delivered_online);
    bool deliverToLocalSession(int from_uid, int to_uid, const Json::Value& text_array);

    bool deliverImageChat(int from_uid, int to_uid, const Json::Value& image_array);
    void persistImageBatch(int from_uid, int to_uid, const Json::Value& image_array,
                           bool delivered_online);
    bool deliverImageToLocalSession(int from_uid, int to_uid, const Json::Value& image_array);

    std::shared_ptr<UserSessionManager> _session_manager;
    std::shared_ptr<UserNodeRouteCache> _route_cache;
    std::shared_ptr<MySqlMgr> _mysql_mgr;
    std::shared_ptr<StatusGrpcClient> _status_client;
    std::shared_ptr<ChatGrpcClient> _chat_client;
    std::shared_ptr<IdGenerator> _id_generator;
    const RuntimeContext& _runtime_context;
};
