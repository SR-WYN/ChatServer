// PersistWorker.h - 异步消息持久化工作者
#pragma once

#include "data.h"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

class MySqlMgr;

struct PersistTask
{
    ChatMessageRecord message;
    bool delivered_online = false;
};

class PersistWorker
{
public:
    explicit PersistWorker(std::shared_ptr<MySqlMgr> mysql_mgr);
    ~PersistWorker();

    void start();
    void stop();
    void enqueue(PersistTask task);

private:
    void run();

    std::shared_ptr<MySqlMgr> _mysql_mgr;
    std::queue<PersistTask> _queue;
    std::mutex _mutex;
    std::condition_variable _cv;
    std::thread _worker;
    bool _running = false;
    bool _stop = false;
};
