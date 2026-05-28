#include "UserDao.h"
#include "DbSession.h"
#include <cppconn/resultset.h>
#include <memory>

namespace
{
const char *kUserSql = "SELECT uid, name, nick, `desc`, sex, icon, email, pwd FROM user";

std::shared_ptr<UserInfo> readUser(sql::ResultSet &rs)
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
} // namespace

std::shared_ptr<UserInfo> UserDao::getUserInfo(int uid)
{
    std::shared_ptr<UserInfo> user;
    DbSession::queryOne(
        std::string(kUserSql) + " WHERE uid = ?",
        [&](sql::PreparedStatement &stmt) {
            stmt.setInt(1, uid);
        },
        [&](sql::ResultSet &rs) {
            user = readUser(rs);
            return true;
        });
    return user;
}

std::shared_ptr<UserInfo> UserDao::getUserInfo(const std::string &name)
{
    std::shared_ptr<UserInfo> user;
    DbSession::queryOne(
        std::string(kUserSql) + " WHERE name = ?",
        [&](sql::PreparedStatement &stmt) {
            stmt.setString(1, name);
        },
        [&](sql::ResultSet &rs) {
            user = readUser(rs);
            return true;
        });
    return user;
}
