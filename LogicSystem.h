#pragma once
#include "Singleton.h"
#include "data.h"
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>

class CSession;
class LogicNode;

typedef std::function<void(std::shared_ptr<CSession>, const short &msg_id,
                           const std::string &msg_data)>
    FunCallBack;

class LogicSystem : public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>;

public:
    ~LogicSystem();
    void postMsgToQue(std::shared_ptr<LogicNode> msg);

private:
    LogicSystem();
    void dealMsg();
    void registerCallBacks();
    void loginHandler(std::shared_ptr<CSession>, const short &msg_id, const std::string &msg_data);
    void searchUserHandler(std::shared_ptr<CSession>, const short &msg_id,
                           const std::string &msg_data);
    bool getBaseInfo(const std::string& base_key, int uid, std::shared_ptr<UserInfo> user_info);
    void getUserByUid(const std::string& uid_str, Json::Value& result);
    void getUserByName(const std::string& name_str, Json::Value& result);
    std::thread _worker_thread;
    std::queue<std::shared_ptr<LogicNode>> _msg_que;
    std::mutex _mutex;
    std::condition_variable _consume;
    bool _b_stop;
    std::map<short, FunCallBack> _fun_callbacks;
};