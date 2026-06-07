#include "IStatusServiceClient.h"
#include "RuntimeContext.h"
#include "Singleton.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <grpcpp/grpcpp.h>
#include <optional>
#include <string>

using message::GetChatServerRsp;
using message::LoginRsp;
using message::GetChatServerReq;
using message::LoginReq;

using grpc::ClientContext;
using grpc::Status;

class StatusConPool;

class StatusGrpcClient : public Singleton<StatusGrpcClient>, public IStatusServiceClient
{
    friend class Singleton<StatusGrpcClient>;

public:
    ~StatusGrpcClient() override;
    GetChatServerRsp getChatServer(int uid);
    LoginRsp login(int uid, std::string token);
    bool registerChatNode(const NodeInfo &node) override;
    bool unregisterChatNode(const NodeInfo &node) override;
    bool heartbeatChatNode(const std::string &name, const std::string &instance_id) override;
    std::optional<UserChatLocation> getUserChatNode(int uid) override;
    std::optional<UserChatLocation> getChatNode(const std::string &name);
    bool bindUserToNode(int uid, const std::string &node_name) override;
    bool unbindUser(int uid);

private:
    StatusGrpcClient(const StatusGrpcClient &) = delete;
    StatusGrpcClient &operator=(const StatusGrpcClient &) = delete;
    StatusGrpcClient();
    std::unique_ptr<StatusConPool> _pool;
};
