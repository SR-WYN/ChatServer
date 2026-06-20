#pragma once
#include "Singleton.h"
#include "ThreadPool.h"
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <vector>

class ThreadPoolMgr : public Singleton<ThreadPoolMgr>
{
    friend class Singleton<ThreadPoolMgr>;

public:
    ~ThreadPoolMgr();

    // ======================== 任务队列线程池 ========================
    void enqueueLogic(std::function<void()> task);
    void enqueueMySql(std::function<void()> task);
    void enqueueGrpcClient(std::function<void()> task);

    // ======================== TCP IO 池 ========================
    // round-robin 返回一个 io_context 给新连接
    boost::asio::io_context &getIoService();

    // ======================== 长生命周期线程 ========================
    // 节点心跳线程（向 StatusServer 定期发送心跳）
    void runNodeHeartbeat(std::function<void()> heartbeatFunc);
    void joinNodeHeartbeat();

private:
    ThreadPoolMgr();

    // ======================== 任务队列线程池 ========================
    // Logic 池：TCP 消息分发与业务逻辑处理
    std::unique_ptr<ThreadPool> _logicPool;
    // MySQL 池：数据库读写操作（聊天记录持久化、用户查询等）
    std::unique_ptr<ThreadPool> _mysqlPool;
    // gRPC 客户端池：向 StatusServer / 其他 ChatServer 的出站 gRPC 调用
    std::unique_ptr<ThreadPool> _grpcClientPool;

    // ======================== TCP IO 池 ========================
    using IOService = boost::asio::io_context;
    using Work = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
    using WorkPtr = std::unique_ptr<Work>;

    std::size_t _ioPoolSize;
    std::vector<IOService> _ioServices;
    std::vector<WorkPtr> _works;
    std::vector<std::thread> _ioThreads;
    std::size_t _nextIoService;

    // ======================== 长生命周期线程 ========================
    std::thread _nodeHeartbeatThread;
};