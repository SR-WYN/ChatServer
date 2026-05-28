#include "FriendDao.h"
#include "DbSession.h"
#include "UserDao.h"
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <memory>
#include <string>

bool FriendDao::addFriendApply(const int &uid, const int &touid,
                               const std::string &apply_alias_name)
{
    return DbSession::exec("INSERT INTO friend_apply (from_uid,to_uid,alias_name) VALUES (?,?,?) "
                           "ON DUPLICATE KEY UPDATE alias_name = VALUES(alias_name)",
                           [&](sql::PreparedStatement &stmt) {
                               stmt.setInt(1, uid);
                               stmt.setInt(2, touid);
                               stmt.setString(3, apply_alias_name);
                           }) >= 0;
}

bool FriendDao::getApplyList(const int &touid, std::vector<std::shared_ptr<ApplyInfo>> &list,
                             int begin, int limit)
{
    return DbSession::queryAll(
        "SELECT apply.from_uid, apply.status, apply.alias_name, user.name, user.nick, user.sex "
        "FROM friend_apply AS apply JOIN user ON apply.from_uid = user.uid "
        "WHERE apply.to_uid = ? AND apply.id > ? ORDER BY apply.id ASC LIMIT ?",
        [&](sql::PreparedStatement &stmt) {
            stmt.setInt(1, touid);
            stmt.setInt(2, begin);
            stmt.setInt(3, limit);
        },
        [](sql::ResultSet &rs) {
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

bool FriendDao::getFriendApplyAlias(int from_uid, int to_uid, std::string &out_alias)
{
    out_alias.clear();
    return DbSession::queryOne(
        "SELECT alias_name FROM friend_apply WHERE from_uid = ? AND to_uid = ? LIMIT 1",
        [&](sql::PreparedStatement &stmt) {
            stmt.setInt(1, from_uid);
            stmt.setInt(2, to_uid);
        },
        [&](sql::ResultSet &rs) {
            out_alias = rs.getString("alias_name");
            return true;
        });
}

bool FriendDao::getFriendAlias(int self_id, int friend_id, std::string &out_alias)
{
    out_alias.clear();
    return DbSession::queryOne(
        "SELECT alias_name FROM friend WHERE self_id = ? AND friend_id = ? LIMIT 1",
        [&](sql::PreparedStatement &stmt) {
            stmt.setInt(1, self_id);
            stmt.setInt(2, friend_id);
        },
        [&](sql::ResultSet &rs) {
            out_alias = rs.getString("alias_name");
            return true;
        });
}

bool FriendDao::authFriendApply(const int &uid, const int &touid)
{
    return DbSession::exec("UPDATE friend_apply SET status = 1 WHERE from_uid = ? AND to_uid = ?",
                           [&](sql::PreparedStatement &stmt) {
                               stmt.setInt(1, uid);
                               stmt.setInt(2, touid);
                           }) > 0;
}

bool FriendDao::addFriend(int applicant_uid, int accepter_uid,
                          const std::string &alias_applicant_for_accepter,
                          const std::string &alias_accepter_for_applicant)
{
    return DbSession::withConn([&](sql::Connection &conn) {
        auto insert = [&](int self_id, int friend_id, const std::string &alias) {
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

bool FriendDao::getFriendList(int uid, std::vector<std::shared_ptr<UserInfo>> &list)
{
    list.clear();
    std::vector<std::pair<int, std::string>> rows;
    if (!DbSession::queryAll(
            "SELECT friend_id, alias_name FROM friend WHERE self_id = ?",
            [&](sql::PreparedStatement &stmt) {
                stmt.setInt(1, uid);
            },
            [](sql::ResultSet &rs) {
                return std::make_pair(rs.getInt("friend_id"), rs.getString("alias_name"));
            },
            rows))
    {
        return false;
    }

    UserDao user_dao;
    for (const auto &row : rows)
    {
        auto base = user_dao.getUserInfo(row.first);
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
