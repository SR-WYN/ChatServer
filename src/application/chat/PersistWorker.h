#pragma once

#include "ChatMessage.h"
#include "Singleton.h"
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

struct PersistTask
{
    ChatMessage message;
    bool delivered_online = false;
};

class PersistWorker : public Singleton<PersistWorker>
{
    friend class Singleton<PersistWorker>;

public:
    ~PersistWorker() override;
    void start();
    void stop();
    void enqueue(PersistTask task);

private:
    PersistWorker();
    PersistWorker(const PersistWorker &) = delete;
    PersistWorker &operator=(const PersistWorker &) = delete;
    void run();

    std::queue<PersistTask> _queue;
    std::mutex _mutex;
    std::condition_variable _cv;
    std::thread _worker;
    bool _running = false;
    bool _stop = false;
};
