#include "Singleton.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <grpcpp/grpcpp.h>

using message::GetChatServerRsp;
using message::LoginRsp;
using message::GetChatServerReq;
using message::LoginReq;

using grpc::ClientContext;
using grpc::Status;

class StatusConPool;

class StatusGrpcClient : public Singleton<StatusGrpcClient>
{
    friend class Singleton<StatusGrpcClient>;

public:
    ~StatusGrpcClient() override;
    GetChatServerRsp getChatServer(int uid);
    LoginRsp login(int uid, std::string token);

private:
    StatusGrpcClient(const StatusGrpcClient &) = delete;
    StatusGrpcClient &operator=(const StatusGrpcClient &) = delete;
    StatusGrpcClient();
    std::unique_ptr<StatusConPool> _pool;
};