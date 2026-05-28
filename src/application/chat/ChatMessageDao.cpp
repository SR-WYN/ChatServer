#include "ChatMessageDao.h"
#include "DbSession.h"
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <iostream>
#include <memory>
#include <string>

namespace
{
bool readMsgId(sql::Connection &conn, int from_uid, const std::string &client_msg_id,
               uint64_t &out_db_id)
{
    auto stmt = std::unique_ptr<sql::PreparedStatement>(conn.prepareStatement(
        "SELECT id FROM chat_message WHERE from_uid = ? AND client_msg_id = ? LIMIT 1"));
    stmt->setInt(1, from_uid);
    stmt->setString(2, client_msg_id);
    auto rs = std::unique_ptr<sql::ResultSet>(stmt->executeQuery());
    if (!rs->next())
    {
        return false;
    }
    out_db_id = static_cast<uint64_t>(rs->getUInt64("id"));
    return true;
}

ChatMessage readChatMsg(sql::ResultSet &rs)
{
    ChatMessage msg;
    msg.id = static_cast<uint64_t>(rs.getUInt64("id"));
    msg.client_msg_id = rs.getString("client_msg_id");
    msg.from_uid = rs.getInt("from_uid");
    msg.to_uid = rs.getInt("to_uid");
    msg.content = rs.getString("content");
    return msg;
}
} // namespace

bool ChatMessageDao::saveMessage(const ChatMessage &msg, uint64_t &out_db_id)
{
    return DbSession::withConn([&](sql::Connection &conn) {
        try
        {
            auto stmt = std::unique_ptr<sql::PreparedStatement>(conn.prepareStatement(
                "INSERT INTO chat_message (client_msg_id, from_uid, to_uid, content) "
                "VALUES (?,?,?,?)"));
            stmt->setString(1, msg.client_msg_id);
            stmt->setInt(2, msg.from_uid);
            stmt->setInt(3, msg.to_uid);
            stmt->setString(4, msg.content);
            stmt->executeUpdate();
            return readMsgId(conn, msg.from_uid, msg.client_msg_id, out_db_id);
        }
        catch (sql::SQLException &e)
        {
            if (e.getErrorCode() != 1062)
            {
                throw;
            }
            return readMsgId(conn, msg.from_uid, msg.client_msg_id, out_db_id);
        }
    });
}

bool ChatMessageDao::existsByClientMsgId(int from_uid, const std::string &client_msg_id)
{
    return DbSession::queryOne(
        "SELECT 1 FROM chat_message WHERE from_uid = ? AND client_msg_id = ? LIMIT 1",
        [&](sql::PreparedStatement &stmt) {
            stmt.setInt(1, from_uid);
            stmt.setString(2, client_msg_id);
        },
        [](sql::ResultSet &) { return true; });
}

bool ChatMessageDao::enqueueOffline(uint64_t message_id, int owner_uid)
{
    return DbSession::exec(
               "INSERT IGNORE INTO chat_offline_inbox (owner_uid, message_id) VALUES (?,?)",
               [&](sql::PreparedStatement &stmt) {
                   stmt.setInt(1, owner_uid);
                   stmt.setUInt64(2, message_id);
               }) >= 0;
}

bool ChatMessageDao::fetchOfflineBatch(int owner_uid, int limit, std::vector<ChatMessage> &out,
                                       std::vector<uint64_t> &out_inbox_ids)
{
    out.clear();
    out_inbox_ids.clear();
    struct OfflineRow
    {
        uint64_t inbox_id;
        ChatMessage msg;
    };
    std::vector<OfflineRow> rows;
    if (!DbSession::queryAll(
            "SELECT inbox.id AS inbox_id, m.id, m.client_msg_id, m.from_uid, m.to_uid, m.content "
            "FROM chat_offline_inbox AS inbox "
            "JOIN chat_message AS m ON inbox.message_id = m.id "
            "WHERE inbox.owner_uid = ? ORDER BY inbox.id ASC LIMIT ?",
            [&](sql::PreparedStatement &stmt) {
                stmt.setInt(1, owner_uid);
                stmt.setInt(2, limit);
            },
            [](sql::ResultSet &rs) {
                OfflineRow row;
                row.inbox_id = static_cast<uint64_t>(rs.getUInt64("inbox_id"));
                row.msg = readChatMsg(rs);
                return row;
            },
            rows))
    {
        return false;
    }
    for (const auto &row : rows)
    {
        out_inbox_ids.push_back(row.inbox_id);
        out.push_back(row.msg);
    }
    return true;
}

bool ChatMessageDao::queryHistory(int self_uid, int peer_uid, uint64_t before_id, int limit,
                                  std::vector<ChatMessage> &out)
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

    std::string sql =
        "SELECT id, client_msg_id, from_uid, to_uid, content FROM chat_message "
        "WHERE ((from_uid = ? AND to_uid = ?) OR (from_uid = ? AND to_uid = ?)) ";
    if (before_id > 0)
    {
        sql += "AND id < ? ";
    }
    sql += "ORDER BY id DESC LIMIT " + std::to_string(limit);

    std::vector<ChatMessage> reversed;
    if (!DbSession::queryAll(
            sql,
            [&](sql::PreparedStatement &stmt) {
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
            [](sql::ResultSet &rs) { return readChatMsg(rs); },
            reversed))
    {
        return false;
    }
    out.assign(reversed.rbegin(), reversed.rend());
    return true;
}

bool ChatMessageDao::removeOfflineByIds(const std::vector<uint64_t> &inbox_ids)
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

    return DbSession::exec(sql, [&](sql::PreparedStatement &stmt) {
               for (size_t i = 0; i < inbox_ids.size(); ++i)
               {
                   stmt.setUInt64(static_cast<unsigned int>(i + 1), inbox_ids[i]);
               }
           }) >= 0;
}
