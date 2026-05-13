#include "MySqlDao.h"
#include "ConfigMgr.h"
#include "MySqlPool.h"
#include "utils.h"
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <iostream>
#include <memory>

MySqlDao::MySqlDao()
{
    auto &cfg = ConfigMgr::getInstance();
    const auto &host = cfg["MySql"]["Host"];
    const auto &port = cfg["MySql"]["Port"];
    const auto &user = cfg["MySql"]["User"];
    const auto &pass = cfg["MySql"]["Passwd"];
    const auto &schema = cfg["MySql"]["Schema"];
    _pool.reset(new MySqlPool(host + ":" + port, user, pass, schema, 5));
};

MySqlDao::~MySqlDao()
{
    _pool->close();
}

std::shared_ptr<UserInfo> MySqlDao::getUserInfo(int uid)
{
    auto con = _pool->getConnection();
    if (con == nullptr)
    {
        return nullptr;
    }

    utils::Defer defer([this, &con]() {
        _pool->returnConnection(std::move(con));
    });

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "SELECT uid, name, nick, `desc`, sex, icon, email, pwd FROM user WHERE uid = ?"));
        pstmt->setInt(1, uid);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> user_ptr = nullptr;
        while (res->next())
        {
            user_ptr.reset(new UserInfo);
            user_ptr->uid = res->getInt("uid");
            user_ptr->pwd = res->getString("pwd");
            user_ptr->email = res->getString("email");
            user_ptr->name = res->getString("name");
            user_ptr->nick = res->getString("nick");
            if (user_ptr->nick.empty())
            {
                user_ptr->nick = user_ptr->name;
            }
            user_ptr->desc = res->getString("desc");
            user_ptr->sex = res->getInt("sex");
            user_ptr->icon = res->getString("icon");
            user_ptr->back.clear();
            user_ptr->alias_name.clear();
            break;
        }
        return user_ptr;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return nullptr;
    }
}

std::shared_ptr<UserInfo> MySqlDao::getUserInfo(std::string name)
{
    auto con = _pool->getConnection();
    if (con == nullptr)
    {
        return nullptr;
    }

    utils::Defer defer([this, &con]() {
        _pool->returnConnection(std::move(con));
    });

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "SELECT uid, name, nick, `desc`, sex, icon, email, pwd FROM user WHERE name = ?"));
        pstmt->setString(1, name);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> user_ptr = nullptr;
        while (res->next())
        {
            user_ptr.reset(new UserInfo);
            user_ptr->uid = res->getInt("uid");
            user_ptr->pwd = res->getString("pwd");
            user_ptr->email = res->getString("email");
            user_ptr->name = res->getString("name");
            user_ptr->desc = res->getString("desc");
            user_ptr->sex = res->getInt("sex");
            user_ptr->icon = res->getString("icon");
            user_ptr->nick = res->getString("nick");
            if (user_ptr->nick.empty())
            {
                user_ptr->nick = user_ptr->name;
            }
            user_ptr->back.clear();
            user_ptr->alias_name.clear();
            break;
        }
        return user_ptr;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return nullptr;
    }
}

bool MySqlDao::addFriendApply(const int &uid, const int &touid, const std::string &apply_alias_name)
{
    auto con = _pool->getConnection();
    if (con == nullptr)
    {
        return false;
    }
    utils::Defer defer([this, &con]() {
        _pool->returnConnection(std::move(con));
    });

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "INSERT INTO friend_apply (from_uid,to_uid,alias_name) VALUES (?,?,?)"
            " ON DUPLICATE KEY UPDATE alias_name = VALUES(alias_name)"));
        pstmt->setInt(1, uid);
        pstmt->setInt(2, touid);
        pstmt->setString(3, apply_alias_name);

        // 执行插入
        int row_affected = pstmt->executeUpdate();
        if (row_affected < 0)
        {
            std::cerr << "Insert friend apply failed" << std::endl;
            return false;
        }

        return true;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }

    return true;
}

bool MySqlDao::getApplyList(const int &touid, std::vector<std::shared_ptr<ApplyInfo>> &list,
                            int begin, int limit)
{
    auto con = _pool->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    utils::Defer defer([this, &con]() {
        _pool->returnConnection(std::move(con));
    });

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "SELECT apply.from_uid, apply.status, apply.alias_name, user.name, user.nick, user.sex "
            "FROM friend_apply AS apply JOIN user ON apply.from_uid = user.uid "
            "WHERE apply.to_uid = ? AND apply.id > ? ORDER BY apply.id ASC LIMIT ?"));
        pstmt->setInt(1, touid);
        pstmt->setInt(2, begin);
        pstmt->setInt(3, limit);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            int from_uid = res->getInt("from_uid");
            int status = res->getInt("status");
            std::string name = res->getString("name");
            std::string nick = res->getString("nick");
            if (nick.empty())
            {
                nick = name;
            }
            int sex = res->getInt("sex");
            std::string apply_alias = res->getString("alias_name");
            list.push_back(
                std::make_shared<ApplyInfo>(from_uid, name, "", "", nick, sex, status, apply_alias));
        }
        return true;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MySqlDao::getFriendApplyAlias(int from_uid, int to_uid, std::string &out_alias)
{
    out_alias.clear();
    auto con = _pool->getConnection();
    if (con == nullptr)
    {
        return false;
    }
    utils::Defer defer([this, &con]() {
        _pool->returnConnection(std::move(con));
    });
    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "SELECT alias_name FROM friend_apply WHERE from_uid = ? AND to_uid = ? LIMIT 1"));
        pstmt->setInt(1, from_uid);
        pstmt->setInt(2, to_uid);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (res->next())
        {
            out_alias = res->getString("alias_name");
            return true;
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
    return false;
}

bool MySqlDao::getFriendAlias(int self_id, int friend_id, std::string &out_alias)
{
    out_alias.clear();
    auto con = _pool->getConnection();
    if (con == nullptr)
    {
        return false;
    }
    utils::Defer defer([this, &con]() {
        _pool->returnConnection(std::move(con));
    });
    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "SELECT alias_name FROM friend WHERE self_id = ? AND friend_id = ? LIMIT 1"));
        pstmt->setInt(1, self_id);
        pstmt->setInt(2, friend_id);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (res->next())
        {
            out_alias = res->getString("alias_name");
            return true;
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
    return false;
}

bool MySqlDao::authFriendApply(const int &uid, const int &touid)
{
    auto con = _pool->getConnection();
    if (con == nullptr)
    {
        return false;
    }
    utils::Defer defer([this, &con] {
        _pool->returnConnection(std::move(con));
    });
    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("UPDATE friend_apply SET status = 1 "
                                        "WHERE from_uid = ? AND to_uid = ?"));
        pstmt->setInt(1, uid);
        pstmt->setInt(2, touid);
        int row_affected = pstmt->executeUpdate();
        return row_affected > 0;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
    return true;
}

bool MySqlDao::addFriend(int applicant_uid, int accepter_uid,
                         const std::string &alias_applicant_for_accepter,
                         const std::string &alias_accepter_for_applicant)
{
    auto con = _pool->getConnection();
    if (con == nullptr)
    {
        return false;
    }
    utils::Defer defer([this, &con] {
        _pool->returnConnection(std::move(con));
    });
    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("INSERT IGNORE INTO friend(self_id,friend_id,alias_name) "
                                        "values (?,?,?)"));
        pstmt->setInt(1, applicant_uid);
        pstmt->setInt(2, accepter_uid);
        pstmt->setString(3, alias_applicant_for_accepter);
        int row_affected = pstmt->executeUpdate();

        std::unique_ptr<sql::PreparedStatement> pstmt2(
            con->_con->prepareStatement("INSERT IGNORE INTO friend(self_id,friend_id,alias_name) "
                                        "values (?,?,?)"));
        pstmt2->setInt(1, accepter_uid);
        pstmt2->setInt(2, applicant_uid);
        pstmt2->setString(3, alias_accepter_for_applicant);
        int row_affected2 = pstmt2->executeUpdate();
        return row_affected > 0 || row_affected2 > 0;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
    return true;
}

bool MySqlDao::getFriendList(int uid, std::vector<std::shared_ptr<UserInfo>> &list)
{
    auto con = _pool->getConnection();
    if (con == nullptr)
    {
        return false;
    }
    utils::Defer defer([this, &con] {
        _pool->returnConnection(std::move(con));
    });

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM friend WHERE self_id = ?"));
        pstmt->setInt(1, uid);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            const int friend_id = res->getInt("friend_id");
            const std::string row_alias = res->getString("alias_name");
            auto base = getUserInfo(friend_id);
            if (base == nullptr)
            {
                continue;
            }
            auto merged = std::make_shared<UserInfo>(*base);
            merged->alias_name = row_alias;
            list.push_back(merged);
        }
        return true;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
    return true;
}