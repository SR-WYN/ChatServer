// PersistWorker.cpp
#include "PersistWorker.h"
#include "Log.h"
#include "MySqlMgr.h"

PersistWorker::PersistWorker(std::shared_ptr<MySqlMgr> mysql_mgr)
    : _mysql_mgr(std::move(mysql_mgr))
{
}

PersistWorker::~PersistWorker()
{
    stop();
}

void PersistWorker::start()
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_running)
    {
        LOGW(LogModule::Chat, "PersistWorker already running");
        return;
    }
    _stop = false;
    _running = true;
    _worker = std::thread([this]() {
        run();
    });
    LOGI(LogModule::Chat, "PersistWorker started");
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
    LOGI(LogModule::Chat, "PersistWorker stopped");
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
    LOGI(LogModule::Chat, "PersistWorker worker thread started");

    while (true)
    {
        PersistTask task;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _cv.wait(lock, [this]() {
                return _stop || !_queue.empty();
            });

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

        if (_mysql_mgr->existsByClientMsgId(task.message.from_uid, task.message.client_msg_id))
        {
            LOGW(LogModule::Chat, "duplicate message ignored | from_uid={} client_msg_id={}",
                 task.message.from_uid, task.message.client_msg_id);
            continue;
        }

        if (!_mysql_mgr->saveMessage(task.message))
        {
            LOGE(LogModule::Chat,
                 "failed to save message | snowflake_id={} from_uid={} to_uid={} client_msg_id={}",
                 task.message.id, task.message.from_uid, task.message.to_uid,
                 task.message.client_msg_id);
            continue;
        }

        LOGI(LogModule::Chat,
             "message persisted | snowflake_id={} from_uid={} to_uid={} client_msg_id={}",
             task.message.id, task.message.from_uid, task.message.to_uid,
             task.message.client_msg_id);

        if (!task.delivered_online)
        {
            _mysql_mgr->enqueueOffline(task.message.id, task.message.to_uid);
            LOGI(LogModule::Chat, "offline message enqueued | snowflake_id={} to_uid={}",
                 task.message.id, task.message.to_uid);
        }
    }

    LOGI(LogModule::Chat, "PersistWorker worker thread exited");
}
