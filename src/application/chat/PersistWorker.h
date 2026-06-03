#pragma once

#include "ChatMessage.h"
#include "Singleton.h"
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

// 持久化任务：封装一条待写入数据库的聊天消息
struct PersistTask
{
    ChatMessage message;           // 聊天消息内容
    bool delivered_online = false; // 消息是否已在线投递（false 则需要存入离线队列）
};

// 异步消息持久化工作者（生产者-消费者模型）
// 负责将聊天消息异步写入 MySQL，主要职责：
// - 接收外部提交的 PersistTask 到内部队列
// - 单后台线程消费队列，执行数据库写入
// - 写入前检查消息去重（基于 from_uid + client_msg_id）
// - 未在线投递的消息自动存入收件人离线队列
class PersistWorker : public Singleton<PersistWorker>
{
    friend class Singleton<PersistWorker>;

public:
    ~PersistWorker() override;

    // 启动后台工作线程
    void start();

    // 停止后台工作线程（等待队列处理完毕）
    void stop();

    // 提交一条持久化任务到队列
    void enqueue(PersistTask task);

private:
    PersistWorker();
    PersistWorker(const PersistWorker &) = delete;
    PersistWorker &operator=(const PersistWorker &) = delete;

    // 工作线程主循环：消费队列中的任务并写入数据库
    void run();

    std::queue<PersistTask> _queue;   // 待持久化的任务队列
    std::mutex _mutex;                 // 保护队列的互斥锁
    std::condition_variable _cv;       // 通知工作线程有新任务或需要停止
    std::thread _worker;               // 后台工作线程
    bool _running = false;             // 工作线程是否正在运行
    bool _stop = false;                // 是否请求停止
};
