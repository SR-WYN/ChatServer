// MySqlMgrImpl.cpp
#include "MySqlMgrImpl.h"
#include "DbSession.h"
#include "MySqlPool.h"
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
    std::shared_ptr<UserInfo> user;
    DbSession::queryOne(
        std::string(kUserSql) + " WHERE uid = ?",
        [&](sql::PreparedStatement& stmt) {
            stmt.setInt(1, uid);
        },
        [&](sql::ResultSet& rs) {
            user = readUser(rs);
            return true;
        });
    return user;
}

std::shared_ptr<UserInfo> MySqlMgrImpl::getUserInfo(const std::string& name)
{
    std::shared_ptr<UserInfo> user;
    DbSession::queryOne(
        std::string(kUserSql) + " WHERE name = ?",
        [&](sql::PreparedStatement& stmt) {
            stmt.setString(1, name);
        },
        [&](sql::ResultSet& rs) {
            user = readUser(rs);
            return true;
        });
    return user;
}

bool MySqlMgrImpl::addFriendApply(int uid, int touid, const std::string& apply_alias_name)
{
    return DbSession::exec("INSERT INTO friend_apply (from_uid,to_uid,alias_name) VALUES (?,?,?) "
                           "ON DUPLICATE KEY UPDATE alias_name = VALUES(alias_name)",
                           [&](sql::PreparedStatement& stmt) {
                               stmt.setInt(1, uid);
                               stmt.setInt(2, touid);
                               stmt.setString(3, apply_alias_name);
                           }) >= 0;
}

bool MySqlMgrImpl::getApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& list,
                                int begin, int limit)
{
    return DbSession::queryAll(
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
}

bool MySqlMgrImpl::getFriendApplyAlias(int from_uid, int to_uid, std::string& out_alias)
{
    out_alias.clear();
    return DbSession::queryOne(
        "SELECT alias_name FROM friend_apply WHERE from_uid = ? AND to_uid = ? LIMIT 1",
        [&](sql::PreparedStatement& stmt) {
            stmt.setInt(1, from_uid);
            stmt.setInt(2, to_uid);
        },
        [&](sql::ResultSet& rs) {
            out_alias = rs.getString("alias_name");
            return true;
        });
}

bool MySqlMgrImpl::getFriendAlias(int self_id, int friend_id, std::string& out_alias)
{
    out_alias.clear();
    return DbSession::queryOne(
        "SELECT alias_name FROM friend WHERE self_id = ? AND friend_id = ? LIMIT 1",
        [&](sql::PreparedStatement& stmt) {
            stmt.setInt(1, self_id);
            stmt.setInt(2, friend_id);
        },
        [&](sql::ResultSet& rs) {
            out_alias = rs.getString("alias_name");
            return true;
        });
}

bool MySqlMgrImpl::authFriendApply(int uid, int touid)
{
    return DbSession::exec("UPDATE friend_apply SET status = 1 WHERE from_uid = ? AND to_uid = ?",
                           [&](sql::PreparedStatement& stmt) {
                               stmt.setInt(1, uid);
                               stmt.setInt(2, touid);
                           }) > 0;
}

bool MySqlMgrImpl::addFriend(int applicant_uid, int accepter_uid,
                             const std::string& alias_applicant_for_accepter,
                             const std::string& alias_accepter_for_applicant)
{
    return DbSession::withConn([&](sql::Connection& conn) {
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
}

bool MySqlMgrImpl::getFriendList(int uid, std::vector<std::shared_ptr<UserInfo>>& list)
{
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
    return true;
}

bool MySqlMgrImpl::saveMessage(const ChatMessageRecord& msg)
{
    return DbSession::withConn([&](sql::Connection& conn) {
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
}

bool MySqlMgrImpl::existsByClientMsgId(int from_uid, const std::string& client_msg_id)
{
    return DbSession::queryOne(
        "SELECT 1 FROM chat_message WHERE from_uid = ? AND client_msg_id = ? LIMIT 1",
        [&](sql::PreparedStatement& stmt) {
            stmt.setInt(1, from_uid);
            stmt.setString(2, client_msg_id);
        },
        [](sql::ResultSet&) {
            return true;
        });
}

bool MySqlMgrImpl::enqueueOffline(uint64_t message_id, int owner_uid)
{
    return DbSession::exec(
               "INSERT IGNORE INTO chat_offline_inbox (owner_uid, message_id) VALUES (?,?)",
               [&](sql::PreparedStatement& stmt) {
                   stmt.setInt(1, owner_uid);
                   stmt.setUInt64(2, message_id);
               }) >= 0;
}

bool MySqlMgrImpl::fetchOfflineBatch(int owner_uid, int limit, std::vector<ChatMessageRecord>& out,
                                     std::vector<uint64_t>& out_inbox_ids)
{
    out.clear();
    out_inbox_ids.clear();
    struct OfflineRow
    {
        uint64_t inbox_id;
        ChatMessageRecord msg;
    };
    std::vector<OfflineRow> rows;
    if (!DbSession::queryAll(
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
            rows))
    {
        return false;
    }
    for (const auto& row : rows)
    {
        out_inbox_ids.push_back(row.inbox_id);
        out.push_back(row.msg);
    }
    return true;
}

bool MySqlMgrImpl::queryHistory(int self_uid, int peer_uid, uint64_t before_id, int limit,
                                std::vector<ChatMessageRecord>& out)
{
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
    if (!DbSession::queryAll(
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
            reversed))
    {
        return false;
    }
    out.assign(reversed.rbegin(), reversed.rend());
    return true;
}

bool MySqlMgrImpl::removeOfflineByIds(const std::vector<uint64_t>& inbox_ids)
{
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

    return DbSession::exec(sql, [&](sql::PreparedStatement& stmt) {
               for (size_t i = 0; i < inbox_ids.size(); ++i)
               {
                   stmt.setUInt64(static_cast<unsigned int>(i + 1), inbox_ids[i]);
               }
           }) >= 0;
}
