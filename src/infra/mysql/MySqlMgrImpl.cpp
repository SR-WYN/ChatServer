// MySqlMgrImpl.cpp
#include "MySqlMgrImpl.h"
#include "DbSession.h"
#include "Log.h"
#include "LogModule.h"
#include "MySqlPool.h"
#include <chrono>
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>

namespace
{
const char* kUserSql = "SELECT uid, name, nick, `desc`, sex, icon, email, pwd FROM user";

std::shared_ptr<UserInfo> readUser(sql::ResultSet& rs)
{
    auto user = std::make_shared<UserInfo>();
    user->uid = rs.getInt("uid");
    user->pwd = rs.getString("pwd");
    user->email = rs.getString("email");
    user->name = rs.getString("name");
    user->nick = rs.getString("nick");
    if (user->nick.empty())
    {
        user->nick = user->name;
    }
    user->desc = rs.getString("desc");
    user->sex = rs.getInt("sex");
    user->icon = rs.getString("icon");
    return user;
}

ChatMessageRecord readChatMsg(sql::ResultSet& rs)
{
    ChatMessageRecord msg;
    msg.id = static_cast<uint64_t>(rs.getUInt64("id"));
    msg.client_msg_id = rs.getString("client_msg_id");
    msg.from_uid = rs.getInt("from_uid");
    msg.to_uid = rs.getInt("to_uid");
    msg.content = rs.getString("content");
    msg.msg_type = rs.getInt("msg_type");
    return msg;
}
} // namespace

MySqlMgrImpl::MySqlMgrImpl(MySqlPool* pool)
    : _pool(pool)
{
    MySqlPool::initOnce();
}

std::shared_ptr<UserInfo> MySqlMgrImpl::getUserInfo(int uid)
{
    const auto start = std::chrono::steady_clock::now();
    std::shared_ptr<UserInfo> user;
    bool ok = DbSession::queryOne(
        std::string(kUserSql) + " WHERE uid = ?",
        [&](sql::PreparedStatement& stmt) {
            stmt.setInt(1, uid);
        },
        [&](sql::ResultSet& rs) {
            user = readUser(rs);
            return true;
        });
    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    if (!ok || !user)
    {
        LOGW(LogModule::Mysql, "getUserInfo(uid) miss uid={} cost={}ms", uid, cost_ms);
        return nullptr;
    }
    LOGI(LogModule::Mysql, "getUserInfo(uid) hit uid={} name={} cost={}ms", uid, user->name,
         cost_ms);
    return user;
}

std::shared_ptr<UserInfo> MySqlMgrImpl::getUserInfo(const std::string& name)
{
    const auto start = std::chrono::steady_clock::now();
    std::shared_ptr<UserInfo> user;
    bool ok = DbSession::queryOne(
        std::string(kUserSql) + " WHERE name = ?",
        [&](sql::PreparedStatement& stmt) {
            stmt.setString(1, name);
        },
        [&](sql::ResultSet& rs) {
            user = readUser(rs);
            return true;
        });
    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    if (!ok || !user)
    {
        LOGW(LogModule::Mysql, "getUserInfo(name) miss name={} cost={}ms", name, cost_ms);
        return nullptr;
    }
    LOGI(LogModule::Mysql, "getUserInfo(name) hit uid={} name={} cost={}ms", user->uid, name,
         cost_ms);
    return user;
}

bool MySqlMgrImpl::addFriendApply(int uid, int touid, const std::string& apply_alias_name)
{
    const auto start = std::chrono::steady_clock::now();
    int rows = DbSession::exec(
        "INSERT INTO friend_apply (from_uid,to_uid,alias_name) VALUES (?,?,?) "
        "ON DUPLICATE KEY UPDATE alias_name = VALUES(alias_name)",
        [&](sql::PreparedStatement& stmt) {
            stmt.setInt(1, uid);
            stmt.setInt(2, touid);
            stmt.setString(3, apply_alias_name);
        });
    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    if (rows < 0)
    {
        LOGE(LogModule::Mysql, "addFriendApply failed uid={} touid={} cost={}ms", uid, touid,
             cost_ms);
        return false;
    }
    LOGI(LogModule::Mysql, "addFriendApply success uid={} touid={} rows={} cost={}ms", uid, touid,
         rows, cost_ms);
    return true;
}

bool MySqlMgrImpl::getApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& list,
                                int begin, int limit)
{
    const auto start = std::chrono::steady_clock::now();
    bool ok = DbSession::queryAll(
        "SELECT apply.from_uid, apply.status, apply.alias_name, user.name, user.nick, user.sex "
        "FROM friend_apply AS apply JOIN user ON apply.from_uid = user.uid "
        "WHERE apply.to_uid = ? AND apply.id > ? ORDER BY apply.id ASC LIMIT ?",
        [&](sql::PreparedStatement& stmt) {
            stmt.setInt(1, touid);
            stmt.setInt(2, begin);
            stmt.setInt(3, limit);
        },
        [](sql::ResultSet& rs) {
            int from_uid = rs.getInt("from_uid");
            int status = rs.getInt("status");
            std::string name = rs.getString("name");
            std::string nick = rs.getString("nick");
            if (nick.empty())
            {
                nick = name;
            }
            int sex = rs.getInt("sex");
            std::string apply_alias = rs.getString("alias_name");
            return std::make_shared<ApplyInfo>(from_uid, name, "", "", nick, sex, status,
                                               apply_alias);
        },
        list);
    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    if (!ok)
    {
        LOGE(LogModule::Mysql, "getApplyList failed touid={} cost={}ms", touid, cost_ms);
        return false;
    }
    LOGI(LogModule::Mysql, "getApplyList success touid={} count={} cost={}ms", touid, list.size(),
         cost_ms);
    return true;
}

bool MySqlMgrImpl::getFriendApplyAlias(int from_uid, int to_uid, std::string& out_alias)
{
    const auto start = std::chrono::steady_clock::now();
    out_alias.clear();
    bool ok = DbSession::queryOne(
        "SELECT alias_name FROM friend_apply WHERE from_uid = ? AND to_uid = ? LIMIT 1",
        [&](sql::PreparedStatement& stmt) {
            stmt.setInt(1, from_uid);
            stmt.setInt(2, to_uid);
        },
        [&](sql::ResultSet& rs) {
            out_alias = rs.getString("alias_name");
            return true;
        });
    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    LOGI(LogModule::Mysql, "getFriendApplyAlias from_uid={} to_uid={} found={} cost={}ms", from_uid,
         to_uid, ok && !out_alias.empty(), cost_ms);
    return ok;
}

bool MySqlMgrImpl::getFriendAlias(int self_id, int friend_id, std::string& out_alias)
{
    const auto start = std::chrono::steady_clock::now();
    out_alias.clear();
    bool ok = DbSession::queryOne(
        "SELECT alias_name FROM friend WHERE self_id = ? AND friend_id = ? LIMIT 1",
        [&](sql::PreparedStatement& stmt) {
            stmt.setInt(1, self_id);
            stmt.setInt(2, friend_id);
        },
        [&](sql::ResultSet& rs) {
            out_alias = rs.getString("alias_name");
            return true;
        });
    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    LOGI(LogModule::Mysql, "getFriendAlias self_id={} friend_id={} found={} cost={}ms", self_id,
         friend_id, ok && !out_alias.empty(), cost_ms);
    return ok;
}

bool MySqlMgrImpl::authFriendApply(int uid, int touid)
{
    const auto start = std::chrono::steady_clock::now();
    int rows = DbSession::exec(
        "UPDATE friend_apply SET status = 1 WHERE from_uid = ? AND to_uid = ?",
        [&](sql::PreparedStatement& stmt) {
            stmt.setInt(1, uid);
            stmt.setInt(2, touid);
        });
    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    if (rows < 0)
    {
        LOGE(LogModule::Mysql, "authFriendApply failed uid={} touid={} cost={}ms", uid, touid,
             cost_ms);
        return false;
    }
    LOGI(LogModule::Mysql, "authFriendApply success uid={} touid={} rows={} cost={}ms", uid, touid,
         rows, cost_ms);
    return rows > 0;
}

bool MySqlMgrImpl::addFriend(int applicant_uid, int accepter_uid,
                             const std::string& alias_applicant_for_accepter,
                             const std::string& alias_accepter_for_applicant)
{
    const auto start = std::chrono::steady_clock::now();
    bool ok = DbSession::withConn([&](sql::Connection& conn) {
        auto insert = [&](int self_id, int friend_id, const std::string& alias) {
            auto stmt = std::unique_ptr<sql::PreparedStatement>(conn.prepareStatement(
                "INSERT IGNORE INTO friend(self_id,friend_id,alias_name) VALUES (?,?,?)"));
            stmt->setInt(1, self_id);
            stmt->setInt(2, friend_id);
            stmt->setString(3, alias);
            return stmt->executeUpdate();
        };
        int n1 = insert(applicant_uid, accepter_uid, alias_applicant_for_accepter);
        int n2 = insert(accepter_uid, applicant_uid, alias_accepter_for_applicant);
        return n1 > 0 || n2 > 0;
    });
    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    if (!ok)
    {
        LOGE(LogModule::Mysql, "addFriend failed applicant={} accepter={} cost={}ms", applicant_uid,
             accepter_uid, cost_ms);
        return false;
    }
    LOGI(LogModule::Mysql, "addFriend success applicant={} accepter={} cost={}ms", applicant_uid,
         accepter_uid, cost_ms);
    return true;
}

bool MySqlMgrImpl::getFriendList(int uid, std::vector<std::shared_ptr<UserInfo>>& list)
{
    const auto start = std::chrono::steady_clock::now();
    list.clear();
    std::vector<std::pair<int, std::string>> rows;
    if (!DbSession::queryAll(
            "SELECT friend_id, alias_name FROM friend WHERE self_id = ?",
            [&](sql::PreparedStatement& stmt) {
                stmt.setInt(1, uid);
            },
            [](sql::ResultSet& rs) {
                return std::make_pair(rs.getInt("friend_id"), rs.getString("alias_name"));
            },
            rows))
    {
        LOGE(LogModule::Mysql, "getFriendList failed uid={}", uid);
        return false;
    }

    for (const auto& row : rows)
    {
        auto base = getUserInfo(row.first);
        if (base == nullptr)
        {
            continue;
        }
        auto merged = std::make_shared<UserInfo>(*base);
        merged->alias_name = row.second;
        list.push_back(merged);
    }
    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    LOGI(LogModule::Mysql, "getFriendList success uid={} count={} cost={}ms", uid, list.size(),
         cost_ms);
    return true;
}

bool MySqlMgrImpl::saveMessage(const ChatMessageRecord& msg)
{
    const auto start = std::chrono::steady_clock::now();
    bool ok = DbSession::withConn([&](sql::Connection& conn) {
        try
        {
            auto stmt = std::unique_ptr<sql::PreparedStatement>(conn.prepareStatement(
                "INSERT INTO chat_message (id, client_msg_id, from_uid, to_uid, content, msg_type) "
                "VALUES (?,?,?,?,?,?)"));
            stmt->setUInt64(1, msg.id);
            stmt->setString(2, msg.client_msg_id);
            stmt->setInt(3, msg.from_uid);
            stmt->setInt(4, msg.to_uid);
            stmt->setString(5, msg.content);
            stmt->setInt(6, msg.msg_type);
            stmt->executeUpdate();
            return true;
        }
        catch (sql::SQLException& e)
        {
            if (e.getErrorCode() == 1062)
            {
                return true;
            }
            throw;
        }
    });
    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    if (!ok)
    {
        LOGE(LogModule::Mysql,
             "saveMessage failed id={} from_uid={} to_uid={} cost={}ms", msg.id, msg.from_uid,
             msg.to_uid, cost_ms);
        return false;
    }
    LOGD(LogModule::Mysql, "saveMessage success id={} from_uid={} to_uid={} cost={}ms", msg.id,
         msg.from_uid, msg.to_uid, cost_ms);
    return true;
}

bool MySqlMgrImpl::existsByClientMsgId(int from_uid, const std::string& client_msg_id)
{
    const auto start = std::chrono::steady_clock::now();
    bool exists = false;
    bool ok = DbSession::queryOne(
        "SELECT 1 FROM chat_message WHERE from_uid = ? AND client_msg_id = ? LIMIT 1",
        [&](sql::PreparedStatement& stmt) {
            stmt.setInt(1, from_uid);
            stmt.setString(2, client_msg_id);
        },
        [](sql::ResultSet&) {
            return true;
        });
    if (ok)
    {
        exists = true;
    }
    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    LOGD(LogModule::Mysql,
         "existsByClientMsgId from_uid={} client_msg_id={} exists={} cost={}ms", from_uid,
         client_msg_id, exists, cost_ms);
    return exists;
}

bool MySqlMgrImpl::enqueueOffline(uint64_t message_id, int owner_uid)
{
    const auto start = std::chrono::steady_clock::now();
    int rows = DbSession::exec(
        "INSERT IGNORE INTO chat_offline_inbox (owner_uid, message_id) VALUES (?,?)",
        [&](sql::PreparedStatement& stmt) {
            stmt.setInt(1, owner_uid);
            stmt.setUInt64(2, message_id);
        });
    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    if (rows < 0)
    {
        LOGE(LogModule::Mysql, "enqueueOffline failed message_id={} owner_uid={} cost={}ms",
             message_id, owner_uid, cost_ms);
        return false;
    }
    LOGD(LogModule::Mysql, "enqueueOffline success message_id={} owner_uid={} cost={}ms",
         message_id, owner_uid, cost_ms);
    return rows >= 0;
}

bool MySqlMgrImpl::fetchOfflineBatch(int owner_uid, int limit, std::vector<ChatMessageRecord>& out,
                                     std::vector<uint64_t>& out_inbox_ids)
{
    const auto start = std::chrono::steady_clock::now();
    out.clear();
    out_inbox_ids.clear();
    struct OfflineRow
    {
        uint64_t inbox_id;
        ChatMessageRecord msg;
    };
    std::vector<OfflineRow> rows;
    bool ok = DbSession::queryAll(
        "SELECT inbox.id AS inbox_id, m.id, m.client_msg_id, m.from_uid, m.to_uid, m.content, "
        "m.msg_type "
        "FROM chat_offline_inbox AS inbox "
        "JOIN chat_message AS m ON inbox.message_id = m.id "
        "WHERE inbox.owner_uid = ? ORDER BY inbox.id ASC LIMIT ?",
        [&](sql::PreparedStatement& stmt) {
            stmt.setInt(1, owner_uid);
            stmt.setInt(2, limit);
        },
        [](sql::ResultSet& rs) {
            OfflineRow row;
            row.inbox_id = static_cast<uint64_t>(rs.getUInt64("inbox_id"));
            row.msg = readChatMsg(rs);
            return row;
        },
        rows);
    for (const auto& row : rows)
    {
        out_inbox_ids.push_back(row.inbox_id);
        out.push_back(row.msg);
    }
    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    if (!ok)
    {
        LOGE(LogModule::Mysql, "fetchOfflineBatch failed owner_uid={} cost={}ms", owner_uid,
             cost_ms);
        return false;
    }
    LOGI(LogModule::Mysql, "fetchOfflineBatch success owner_uid={} count={} cost={}ms", owner_uid,
         out.size(), cost_ms);
    return true;
}

bool MySqlMgrImpl::queryHistory(int self_uid, int peer_uid, uint64_t before_id, int limit,
                                std::vector<ChatMessageRecord>& out)
{
    const auto start = std::chrono::steady_clock::now();
    out.clear();
    if (limit <= 0)
    {
        return true;
    }
    if (limit > 200)
    {
        limit = 200;
    }

    std::string sql = "SELECT id, client_msg_id, from_uid, to_uid, content, msg_type FROM chat_message "
                      "WHERE ((from_uid = ? AND to_uid = ?) OR (from_uid = ? AND to_uid = ?)) ";
    if (before_id > 0)
    {
        sql += "AND id < ? ";
    }
    sql += "ORDER BY id DESC LIMIT " + std::to_string(limit);

    std::vector<ChatMessageRecord> reversed;
    bool ok = DbSession::queryAll(
        sql,
        [&](sql::PreparedStatement& stmt) {
            int idx = 1;
            stmt.setInt(idx++, self_uid);
            stmt.setInt(idx++, peer_uid);
            stmt.setInt(idx++, peer_uid);
            stmt.setInt(idx++, self_uid);
            if (before_id > 0)
            {
                stmt.setUInt64(idx++, before_id);
            }
        },
        [](sql::ResultSet& rs) {
            return readChatMsg(rs);
        },
        reversed);
    out.assign(reversed.rbegin(), reversed.rend());
    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    if (!ok)
    {
        LOGE(LogModule::Mysql, "queryHistory failed self_uid={} peer_uid={} cost={}ms", self_uid,
             peer_uid, cost_ms);
        return false;
    }
    LOGI(LogModule::Mysql, "queryHistory success self_uid={} peer_uid={} before_id={} count={} "
                           "cost={}ms",
         self_uid, peer_uid, before_id, out.size(), cost_ms);
    return true;
}

bool MySqlMgrImpl::removeOfflineByIds(const std::vector<uint64_t>& inbox_ids)
{
    const auto start = std::chrono::steady_clock::now();
    if (inbox_ids.empty())
    {
        return true;
    }

    std::string sql = "DELETE FROM chat_offline_inbox WHERE id IN (";
    for (size_t i = 0; i < inbox_ids.size(); ++i)
    {
        if (i > 0)
        {
            sql += ",";
        }
        sql += "?";
    }
    sql += ")";

    int rows = DbSession::exec(sql, [&](sql::PreparedStatement& stmt) {
        for (size_t i = 0; i < inbox_ids.size(); ++i)
        {
            stmt.setUInt64(static_cast<unsigned int>(i + 1), inbox_ids[i]);
        }
    });
    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    if (rows < 0)
    {
        LOGE(LogModule::Mysql, "removeOfflineByIds failed count={} cost={}ms", inbox_ids.size(),
             cost_ms);
        return false;
    }
    LOGI(LogModule::Mysql, "removeOfflineByIds success count={} rows={} cost={}ms",
         inbox_ids.size(), rows, cost_ms);
    return rows >= 0;
}
