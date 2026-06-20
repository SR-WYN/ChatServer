#include "ThreadPoolMgr.h"

ThreadPoolMgr::ThreadPoolMgr()
    : _ioPoolSize(2), _ioServices(_ioPoolSize), _works(_ioPoolSize), _nextIoService(0)
{
    // Logic 池 5 线程：TCP 消息分发与业务逻辑处理
    _logicPool = std::make_unique<ThreadPool>(5);
    // MySQL 池 2 线程：数据库读写操作
    _mysqlPool = std::make_unique<ThreadPool>(2);
    // gRPC 客户端池 2 线程：出站 gRPC 调用
    _grpcClientPool = std::make_unique<ThreadPool>(2);

    // TCP IO 池 2 线程：连接级 io_context::run()
    for (std::size_t i = 0; i < _ioPoolSize; ++i)
    {
        _works[i] = std::make_unique<Work>(boost::asio::make_work_guard(_ioServices[i]));
    }
    for (std::size_t i = 0; i < _ioPoolSize; ++i)
    {
        _ioThreads.emplace_back([this, i]() {
            _ioServices[i].run();
        });
    }
}

ThreadPoolMgr::~ThreadPoolMgr()
{
    for (auto &work : _works)
    {
        work->get_executor().context().stop();
        work.reset();
    }
    for (auto &t : _ioThreads)
    {
        if (t.joinable())
            t.join();
    }
}

boost::asio::io_context &ThreadPoolMgr::getIoService()
{
    auto &service = _ioServices[_nextIoService++];
    if (_nextIoService == _ioPoolSize)
        _nextIoService = 0;
    return service;
}

void ThreadPoolMgr::enqueueLogic(std::function<void()> task)
{
    _logicPool->enqueue(std::move(task));
}

void ThreadPoolMgr::enqueueMySql(std::function<void()> task)
{
    _mysqlPool->enqueue(std::move(task));
}

void ThreadPoolMgr::enqueueGrpcClient(std::function<void()> task)
{
    _grpcClientPool->enqueue(std::move(task));
}

void ThreadPoolMgr::runNodeHeartbeat(std::function<void()> heartbeatFunc)
{
    _nodeHeartbeatThread = std::thread(std::move(heartbeatFunc));
}

void ThreadPoolMgr::joinNodeHeartbeat()
{
    if (_nodeHeartbeatThread.joinable())
        _nodeHeartbeatThread.join();
}