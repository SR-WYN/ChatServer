#include "MySqlPool.h"
#include "ConfigMgr.h"
#include "Log.h"
#include "LogModule.h"
#include "utils.h"
#include <boost/asio/steady_timer.hpp>
#include <cppconn/exception.h>
#include <cppconn/statement.h>
#include <memory>
#include <mutex>
#include <mysql_driver.h>

SqlConnection::SqlConnection(sql::Connection *con, int64_t lasttime)
    : _con(con), _last_oper_time(lasttime)
{
}

MySqlPool::MySqlPool() = default;

void MySqlPool::initOnce()
{
    auto &pool = getInstance();
    std::lock_guard<std::mutex> lock(pool._init_mutex);
    if (pool._initialized.load())
    {
        return;
    }
    auto &cfg = ConfigMgr::getInstance();
    const auto &host = cfg["MySql"]["Host"];
    const auto &port = cfg["MySql"]["Port"];
    const auto &user = cfg["MySql"]["User"];
    const auto &pass = cfg["MySql"]["Passwd"];
    const auto &schema = cfg["MySql"]["Schema"];
    LOGI(LogModule::Mysql, "MySqlPool initOnce host={} port={} schema={} user={}", host, port,
         schema, user);
    pool.initPool(host + ":" + port, user, pass, schema, 5);
    pool._initialized.store(true);
}

void MySqlPool::initPool(const std::string &url, const std::string &user, const std::string &pass,
                         const std::string &schema, int poolSize)
{
    _url = url;
    _user = user;
    _pass = pass;
    _schema = schema;
    _pool_size = poolSize;
    _stop.store(false);
    _fail_count.store(0);
    try
    {
        for (int i = 0; i < _pool_size; i++)
        {
            sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();
            auto *con = driver->connect(_url, _user, _pass);
            con->setSchema(_schema);
            // 获取当前时间戳
            auto currentTime = std::chrono::system_clock::now().time_since_epoch();
            // 将时间戳转换为秒
            long long timestamp =
                std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
            _pool.push(std::make_unique<SqlConnection>(con, timestamp));
        }
        LOGI(LogModule::Mysql, "MySqlPool initialized with {} connections to {}", _pool_size, _url);
    }
    catch (sql::SQLException &e)
    {
        LOGE(LogModule::Mysql, "MySqlPool initPool failed url={} error={}", _url, e.what());
    }
}

void MySqlPool::startHealthCheck(boost::asio::io_context &ioc)
{
    auto timer = std::make_shared<boost::asio::steady_timer>(ioc);

    std::function<void()> doCheck;
    doCheck = [this, timer, &doCheck]() {
        if (_stop)
            return;
        checkConnection();
        timer->expires_after(std::chrono::seconds(60));
        timer->async_wait([&doCheck](const boost::system::error_code &ec) {
            if (!ec)
                doCheck();
        });
    };
    doCheck();
    LOGI(LogModule::Mysql, "MySqlPool health check started interval=60s");
}

MySqlPool::~MySqlPool()
{
    close();

    std::lock_guard<std::mutex> lock(_mutex);
    while (!_pool.empty())
    {
        _pool.pop();
    }
}

std::unique_ptr<SqlConnection> MySqlPool::getConnection()
{
    const auto start = std::chrono::steady_clock::now();
    std::unique_lock<std::mutex> lock(_mutex);
    bool got = _cond.wait_for(lock, std::chrono::seconds(5), [this]() {
        if (_stop)
        {
            return true;
        }
        return !_pool.empty();
    });

    if (!got)
    {
        LOGE(LogModule::Mysql, "getConnection: timeout waiting for available connection");
        return nullptr;
    }

    if (_stop)
    {
        return nullptr;
    }

    std::unique_ptr<SqlConnection> con(std::move(_pool.front()));
    _pool.pop();

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    LOGD(LogModule::Mysql, "getConnection: acquired connection cost={}ms pool_size={}", cost_ms,
         _pool.size());
    return con;
}

void MySqlPool::returnConnection(std::unique_ptr<SqlConnection> con)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_stop)
    {
        return;
    }
    _pool.push(std::move(con));
    _cond.notify_one();
}

void MySqlPool::close()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _stop = true;
    _cond.notify_all();
    LOGI(LogModule::Mysql, "MySqlPool closed");
}

void MySqlPool::checkConnection()
{
    size_t targetCount;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        targetCount = _pool.size();
    }

    auto now = std::chrono::system_clock::now().time_since_epoch();
    long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(now).count();

    int checked = 0;
    int renewed = 0;
    for (size_t i = 0; i < targetCount; ++i)
    {
        std::unique_ptr<SqlConnection> con;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_pool.empty())
                break;
            con = std::move(_pool.front());
            _pool.pop();
        }

        bool healthy = true;
        utils::Defer defer([&] {
            if (healthy && con)
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _pool.push(std::move(con));
                _cond.notify_one();
            }
        });

        // 检查活跃度
        if (timestamp - con->_last_oper_time >= 5)
        {
            try
            {
                std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
                stmt->executeQuery("SELECT 1");
                con->_last_oper_time = timestamp;
                ++checked;
            }
            catch (sql::SQLException &e)
            {
                healthy = false; // 标记为坏连接，DEFER 此时不会将其还回池子
                _fail_count++;
                LOGW(LogModule::Mysql, "checkConnection: bad connection detected: {}", e.what());
            }
        }
    }

    // 重连补偿：确保把缺额补齐，去掉那个多余的 break
    while (_fail_count > 0)
    {
        if (reconnect(timestamp))
        {
            _fail_count--;
            ++renewed;
        }
        else
        {
            LOGE(LogModule::Mysql, "checkConnection: reconnect failed, remaining fail_count={}",
                 _fail_count.load());
            break; // 确实连不上了（如网络断开），退出等下次检查
        }
    }

    LOGI(LogModule::Mysql, "checkConnection done checked={} renewed={} fail_count={}", checked,
         renewed, _fail_count.load());
}

bool MySqlPool::reconnect(long long timestamp)
{
    try
    {
        sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();
        auto *con = driver->connect(_url, _user, _pass);
        con->setSchema(_schema);

        auto newCon = std::make_unique<SqlConnection>(con, timestamp);
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _pool.push(std::move(newCon));
        }

        LOGI(LogModule::Mysql, "reconnect: new connection added to pool");
        return true;
    }
    catch (sql::SQLException &e)
    {
        LOGE(LogModule::Mysql, "reconnect failed url={} error={}", _url, e.what());
        return false;
    }
}
