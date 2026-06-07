#include "PersistWorker.h"
#include "Log.h"
#include "MySqlMgr.h"

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
        LOGW(LogModule::Chat, "PersistWorker already running");
        return;
    }
    _stop = false;
    _running = true;
    _worker = std::thread([this]() { run(); });
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
            // 等待队列有任务或收到停止信号
            std::unique_lock<std::mutex> lock(_mutex);
            _cv.wait(lock, [this]() { return _stop || !_queue.empty(); });

            // 收到停止信号且队列已空，退出循环
            if (_stop && _queue.empty())
            {
                break;
            }
            // 被虚假唤醒但队列仍为空，继续等待
            if (_queue.empty())
            {
                continue;
            }

            // 取出队首任务
            task = std::move(_queue.front());
            _queue.pop();
        }

        // ---- 1. 去重检查 ----
        auto &repo = MySqlMgr::getInstance().chatMessages();
        if (repo.existsByClientMsgId(task.message.from_uid, task.message.client_msg_id))
        {
            LOGW(LogModule::Chat,
                 "duplicate message ignored | from_uid={} client_msg_id={}",
                 task.message.from_uid, task.message.client_msg_id);
            continue;
        }

        // ---- 2. 写入数据库（雪花 ID 已在调用方生成） ----
        if (!repo.saveMessage(task.message))
        {
            LOGE(LogModule::Chat,
                 "failed to save message | snowflake_id={} from_uid={} to_uid={} client_msg_id={}",
                 task.message.id, task.message.from_uid, task.message.to_uid, task.message.client_msg_id);
            continue;
        }

        LOGI(LogModule::Chat,
             "message persisted | snowflake_id={} from_uid={} to_uid={} client_msg_id={}",
             task.message.id, task.message.from_uid, task.message.to_uid, task.message.client_msg_id);

        // ---- 3. 未在线投递，存入离线队列 ----
        if (!task.delivered_online)
        {
            repo.enqueueOffline(task.message.id, task.message.to_uid);
            LOGI(LogModule::Chat,
                 "offline message enqueued | snowflake_id={} to_uid={}",
                 task.message.id, task.message.to_uid);
        }
    }

    LOGI(LogModule::Chat, "PersistWorker worker thread exited");
}
