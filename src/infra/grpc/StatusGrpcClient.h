#include "RuntimeContext.h"
#include "Singleton.h"
#include "StatusServiceClient.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <grpcpp/grpcpp.h>
#include <optional>
#include <string>

using message::GetChatServerReq;
using message::GetChatServerRsp;

using grpc::ClientContext;
using grpc::Status;

class StatusConPool;

class StatusGrpcClient : public Singleton<StatusGrpcClient>, public StatusServiceClient
{
    friend class Singleton<StatusGrpcClient>;

public:
    ~StatusGrpcClient() override;
    GetChatServerRsp getChatServer(int uid);
    bool registerChatNode(const NodeInfo &node) override;
    bool unregisterChatNode(const NodeInfo &node) override;
    bool heartbeatChatNode(const std::string &name, const std::string &instance_id) override;
    std::optional<UserChatLocation> getUserChatNode(int uid) override;
    bool bindUserToNode(int uid, const std::string &node_name) override;
    bool unbindUser(int uid);

private:
    StatusGrpcClient(const StatusGrpcClient &) = delete;
    StatusGrpcClient &operator=(const StatusGrpcClient &) = delete;
    StatusGrpcClient();
    std::unique_ptr<StatusConPool> _pool;
};
