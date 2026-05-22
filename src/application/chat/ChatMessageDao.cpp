#include "ChatMessageDao.h"
#include "MySqlPool.h"
#include "utils.h"
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <iostream>

bool ChatMessageDao::saveMessage(const ChatMessage &msg, uint64_t &out_db_id)
{
    auto &pool = MySqlPool::getInstance();
    auto con = pool.getConnection();
    if (con == nullptr)
    {
        return false;
    }

    utils::Defer defer([&pool, &con]() { pool.returnConnection(std::move(con)); });

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "INSERT INTO chat_message (client_msg_id, from_uid, to_uid, content) VALUES (?,?,?,?)"));
        pstmt->setString(1, msg.client_msg_id);
        pstmt->setInt(2, msg.from_uid);
        pstmt->setInt(3, msg.to_uid);
        pstmt->setString(4, msg.content);
        pstmt->executeUpdate();

        std::unique_ptr<sql::PreparedStatement> sel(con->_con->prepareStatement(
            "SELECT id FROM chat_message WHERE from_uid = ? AND client_msg_id = ? LIMIT 1"));
        sel->setInt(1, msg.from_uid);
        sel->setString(2, msg.client_msg_id);
        std::unique_ptr<sql::ResultSet> res(sel->executeQuery());
        if (res->next())
        {
            out_db_id = static_cast<uint64_t>(res->getUInt64("id"));
            return true;
        }
        return false;
    }
    catch (sql::SQLException &e)
    {
        if (e.getErrorCode() == 1062)
        {
            std::unique_ptr<sql::PreparedStatement> sel(con->_con->prepareStatement(
                "SELECT id FROM chat_message WHERE from_uid = ? AND client_msg_id = ? LIMIT 1"));
            sel->setInt(1, msg.from_uid);
            sel->setString(2, msg.client_msg_id);
            std::unique_ptr<sql::ResultSet> res(sel->executeQuery());
            if (res->next())
            {
                out_db_id = static_cast<uint64_t>(res->getUInt64("id"));
                return true;
            }
        }
        std::cerr << "saveMessage SQLException: " << e.what() << std::endl;
        return false;
    }
}

bool ChatMessageDao::existsByClientMsgId(int from_uid, const std::string &client_msg_id)
{
    auto &pool = MySqlPool::getInstance();
    auto con = pool.getConnection();
    if (con == nullptr)
    {
        return false;
    }

    utils::Defer defer([&pool, &con]() { pool.returnConnection(std::move(con)); });

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "SELECT 1 FROM chat_message WHERE from_uid = ? AND client_msg_id = ? LIMIT 1"));
        pstmt->setInt(1, from_uid);
        pstmt->setString(2, client_msg_id);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        return res->next();
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "existsByClientMsgId SQLException: " << e.what() << std::endl;
        return false;
    }
}

bool ChatMessageDao::enqueueOffline(uint64_t message_id, int owner_uid)
{
    auto &pool = MySqlPool::getInstance();
    auto con = pool.getConnection();
    if (con == nullptr)
    {
        return false;
    }

    utils::Defer defer([&pool, &con]() { pool.returnConnection(std::move(con)); });

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "INSERT IGNORE INTO chat_offline_inbox (owner_uid, message_id) VALUES (?,?)"));
        pstmt->setInt(1, owner_uid);
        pstmt->setUInt64(2, message_id);
        pstmt->executeUpdate();
        return true;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "enqueueOffline SQLException: " << e.what() << std::endl;
        return false;
    }
}

bool ChatMessageDao::fetchOfflineBatch(int owner_uid, int limit, std::vector<ChatMessage> &out,
                                       std::vector<uint64_t> &out_inbox_ids)
{
    out.clear();
    out_inbox_ids.clear();

    auto &pool = MySqlPool::getInstance();
    auto con = pool.getConnection();
    if (con == nullptr)
    {
        return false;
    }

    utils::Defer defer([&pool, &con]() { pool.returnConnection(std::move(con)); });

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "SELECT inbox.id AS inbox_id, m.id, m.client_msg_id, m.from_uid, m.to_uid, m.content "
            "FROM chat_offline_inbox AS inbox "
            "JOIN chat_message AS m ON inbox.message_id = m.id "
            "WHERE inbox.owner_uid = ? ORDER BY inbox.id ASC LIMIT ?"));
        pstmt->setInt(1, owner_uid);
        pstmt->setInt(2, limit);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            out_inbox_ids.push_back(static_cast<uint64_t>(res->getUInt64("inbox_id")));
            ChatMessage msg;
            msg.id = static_cast<uint64_t>(res->getUInt64("id"));
            msg.client_msg_id = res->getString("client_msg_id");
            msg.from_uid = res->getInt("from_uid");
            msg.to_uid = res->getInt("to_uid");
            msg.content = res->getString("content");
            out.push_back(std::move(msg));
        }
        return true;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "fetchOfflineBatch SQLException: " << e.what() << std::endl;
        return false;
    }
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

    auto &pool = MySqlPool::getInstance();
    auto con = pool.getConnection();
    if (con == nullptr)
    {
        return false;
    }

    utils::Defer defer([&pool, &con]() { pool.returnConnection(std::move(con)); });

    try
    {
        std::string sql =
            "SELECT id, client_msg_id, from_uid, to_uid, content FROM chat_message "
            "WHERE ((from_uid = ? AND to_uid = ?) OR (from_uid = ? AND to_uid = ?)) ";
        if (before_id > 0)
        {
            sql += "AND id < ? ";
        }
        sql += "ORDER BY id DESC LIMIT ?";

        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(sql));
        int idx = 1;
        pstmt->setInt(idx++, self_uid);
        pstmt->setInt(idx++, peer_uid);
        pstmt->setInt(idx++, peer_uid);
        pstmt->setInt(idx++, self_uid);
        if (before_id > 0)
        {
            pstmt->setUInt64(idx++, before_id);
        }
        pstmt->setInt(idx++, limit);

        std::vector<ChatMessage> reversed;
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            ChatMessage msg;
            msg.id = static_cast<uint64_t>(res->getUInt64("id"));
            msg.client_msg_id = res->getString("client_msg_id");
            msg.from_uid = res->getInt("from_uid");
            msg.to_uid = res->getInt("to_uid");
            msg.content = res->getString("content");
            reversed.push_back(std::move(msg));
        }
        out.assign(reversed.rbegin(), reversed.rend());
        return true;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "queryHistory SQLException: " << e.what() << std::endl;
        return false;
    }
}

bool ChatMessageDao::removeOfflineByIds(const std::vector<uint64_t> &inbox_ids)
{
    if (inbox_ids.empty())
    {
        return true;
    }

    auto &pool = MySqlPool::getInstance();
    auto con = pool.getConnection();
    if (con == nullptr)
    {
        return false;
    }

    utils::Defer defer([&pool, &con]() { pool.returnConnection(std::move(con)); });

    try
    {
        std::string placeholders;
        for (size_t i = 0; i < inbox_ids.size(); ++i)
        {
            if (i > 0)
            {
                placeholders += ",";
            }
            placeholders += "?";
        }
        const std::string sql =
            "DELETE FROM chat_offline_inbox WHERE id IN (" + placeholders + ")";
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(sql));
        for (size_t i = 0; i < inbox_ids.size(); ++i)
        {
            pstmt->setUInt64(static_cast<unsigned int>(i + 1), inbox_ids[i]);
        }
        pstmt->executeUpdate();
        return true;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "removeOfflineByIds SQLException: " << e.what() << std::endl;
        return false;
    }
}
