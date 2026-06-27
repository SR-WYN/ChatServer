#pragma once

#include "Log.h"
#include "LogModule.h"
#include "MySqlPool.h"
#include "utils.h"
#include <chrono>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class DbSession
{
public:
    using BindFn = std::function<void(sql::PreparedStatement &)>;

    template <typename Fn>
    static bool withConn(Fn &&fn)
    {
        auto &pool = MySqlPool::getInstance();
        const auto start = std::chrono::steady_clock::now();
        auto con = pool.getConnection();
        if (!con)
        {
            LOGE(LogModule::Mysql, "DbSession::withConn failed to acquire connection");
            return false;
        }
        utils::Defer defer([&]() {
            pool.returnConnection(std::move(con));
        });
        try
        {
            bool result = fn(*con->_con);
            const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::steady_clock::now() - start)
                                     .count();
            LOGD(LogModule::Mysql, "DbSession::withConn result={} cost={}ms", result, cost_ms);
            return result;
        }
        catch (sql::SQLException &e)
        {
            const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::steady_clock::now() - start)
                                     .count();
            LOGE(LogModule::Mysql, "DbSession::withConn SQLException: {} cost={}ms", e.what(),
                 cost_ms);
            return false;
        }
    }

    static int exec(const std::string &sql, BindFn bind = nullptr)
    {
        const auto start = std::chrono::steady_clock::now();
        int rows = -1;
        withConn([&](sql::Connection &conn) {
            auto stmt = std::unique_ptr<sql::PreparedStatement>(conn.prepareStatement(sql));
            if (bind)
            {
                bind(*stmt);
            }
            rows = stmt->executeUpdate();
            return rows >= 0;
        });
        const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::steady_clock::now() - start)
                                 .count();
        LOGD(LogModule::Mysql, "DbSession::exec rows={} cost={}ms", rows, cost_ms);
        return rows;
    }

    template <typename MapFn>
    static bool queryOne(const std::string &sql, BindFn bind, MapFn &&map)
    {
        const auto start = std::chrono::steady_clock::now();
        bool found = false;
        bool ok = withConn([&](sql::Connection &conn) {
            auto stmt = std::unique_ptr<sql::PreparedStatement>(conn.prepareStatement(sql));
            if (bind)
            {
                bind(*stmt);
            }
            auto rs = std::unique_ptr<sql::ResultSet>(stmt->executeQuery());
            if (!rs->next())
            {
                return true;
            }
            found = true;
            return map(*rs);
        });
        const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::steady_clock::now() - start)
                                 .count();
        LOGD(LogModule::Mysql, "DbSession::queryOne found={} ok={} cost={}ms", found, ok, cost_ms);
        return ok && found;
    }

    template <typename MapFn, typename T>
    static bool queryAll(const std::string &sql, BindFn bind, MapFn &&map, std::vector<T> &out)
    {
        const auto start = std::chrono::steady_clock::now();
        out.clear();
        bool ok = withConn([&](sql::Connection &conn) {
            auto stmt = std::unique_ptr<sql::PreparedStatement>(conn.prepareStatement(sql));
            if (bind)
            {
                bind(*stmt);
            }
            auto rs = std::unique_ptr<sql::ResultSet>(stmt->executeQuery());
            while (rs->next())
            {
                out.push_back(map(*rs));
            }
            return true;
        });
        const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::steady_clock::now() - start)
                                 .count();
        LOGD(LogModule::Mysql, "DbSession::queryAll count={} ok={} cost={}ms", out.size(), ok,
             cost_ms);
        return ok;
    }
};
