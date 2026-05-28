#include "PersistWorker.h"
#include "MySqlMgr.h"
#include <iostream>

PersistWorker::PersistWorker() = default;

PersistWorker::~PersistWorker()
{
    stop();
}

void PersistWorker::start()
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_running)
    {
        return;
    }
    _stop = false;
    _running = true;
    _worker = std::thread([this]() { run(); });
}

void PersistWorker::stop()
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_running)
        {
            return;
        }
        _stop = true;
    }
    _cv.notify_all();
    if (_worker.joinable())
    {
        _worker.join();
    }
    _running = false;
}

void PersistWorker::enqueue(PersistTask task)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(std::move(task));
    }
    _cv.notify_one();
}

void PersistWorker::run()
{
    while (true)
    {
        PersistTask task;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _cv.wait(lock, [this]() { return _stop || !_queue.empty(); });
            if (_stop && _queue.empty())
            {
                break;
            }
            if (_queue.empty())
            {
                continue;
            }
            task = std::move(_queue.front());
            _queue.pop();
        }

        auto &repo = MySqlMgr::getInstance().chatMessages();
        if (repo.existsByClientMsgId(task.message.from_uid, task.message.client_msg_id))
        {
            continue;
        }

        uint64_t db_id = 0;
        if (!repo.saveMessage(task.message, db_id))
        {
            continue;
        }

        if (!task.delivered_online)
        {
            repo.enqueueOffline(db_id, task.message.to_uid);
        }
    }
}
