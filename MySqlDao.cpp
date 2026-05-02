#include "MySqlDao.h"
#include "ConfigMgr.h"
#include "MySqlPool.h"
#include "utils.h"
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <iostream>

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
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("SELECT * FROM user WHERE uid = ?"));
        pstmt->setInt(1, uid);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> user_ptr = nullptr;
        while (res->next())
        {
            user_ptr.reset(new UserInfo);
            user_ptr->pwd = res->getString("pwd");
            user_ptr->email = res->getString("email");
            user_ptr->name = res->getString("name");
            user_ptr->uid = uid;
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
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("SELECT * FROM user WHERE name = ?"));
        pstmt->setString(1, name);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> user_ptr = nullptr;
        while (res->next())
        {
            user_ptr.reset(new UserInfo);
            user_ptr->pwd = res->getString("pwd");
            user_ptr->email = res->getString("email");
            user_ptr->name = res->getString("name");
            user_ptr->uid = res->getInt("uid");
            user_ptr->nick = res->getString("nick");
            user_ptr->desc = res->getString("desc");
            user_ptr->sex = res->getInt("sex");
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

bool MySqlDao::addFriendApply(const int &uid, const int &touid)
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
            "INSERT INTO friend_apply (from_uid,to_uid) values (?,?)"
            "ON DUPLICATE KEY UPDATE from_uid = from_uid, to_uid = to_uid"));
        pstmt->setInt(1, uid);
        pstmt->setInt(2, touid);
        
        //执行插入
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

bool MySqlDao::getApplyList(const int& touid,std::vector<std::shared_ptr<ApplyInfo>> &list,int begin,int limit)
{
    auto con = _pool->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    utils::Defer defer ([this,&con](){
        _pool->returnConnection(std::move(con));
    });

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
            "SELECT apply.from_uid, apply.status, user.name, "
            "user.nick, user.sex FROM friend_apply AS apply JOIN user ON apply.from_uid = user.uid "
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
            int sex = res->getInt("sex");
            list.push_back(std::make_shared<ApplyInfo>(from_uid, name, "", "", nick, sex, status));
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