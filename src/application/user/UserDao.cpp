#include "UserDao.h"
#include "MySqlPool.h"
#include "utils.h"
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <iostream>

std::shared_ptr<UserInfo> UserDao::getUserInfo(int uid)
{
    auto &pool = MySqlPool::getInstance();
    auto con = pool.getConnection();
    if (con == nullptr)
    {
        return nullptr;
    }

    utils::Defer defer([&pool, &con]() { pool.returnConnection(std::move(con)); });

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

std::shared_ptr<UserInfo> UserDao::getUserInfo(const std::string &name)
{
    auto &pool = MySqlPool::getInstance();
    auto con = pool.getConnection();
    if (con == nullptr)
    {
        return nullptr;
    }

    utils::Defer defer([&pool, &con]() { pool.returnConnection(std::move(con)); });

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
