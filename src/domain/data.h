#pragma once
#include <string>

struct UserInfo
{
    UserInfo()
        : name(""), pwd(""), uid(0), email(""), nick(""), desc(""), sex(0), icon(""), back(""),
          alias_name("")
    {
    }

    std::string name;
    std::string pwd;
    int uid;
    std::string email;
    std::string nick;
    std::string desc;
    int sex;
    std::string icon;
    std::string back;
    /** 仅好友列表合并行使用：self 对 friend 的备注（friend 表）；从 user 表加载时应保持为空 */
    std::string alias_name;
};

struct ApplyInfo
{
    ApplyInfo(int uid, const std::string &name, const std::string &desc, const std::string &icon,
              const std::string &nick, int sex, int status,
              const std::string &alias_name = std::string())
        : _uid(uid), _name(name), _desc(desc), _icon(icon), _nick(nick), _sex(sex), _status(status),
          alias_name(alias_name)
    {
    }

    int _uid;
    std::string _name;
    std::string _desc;
    std::string _icon;
    std::string _nick;
    int _sex;
    int _status;
    /** friend_apply.alias_name：申请人对被申请人的备注 */
    std::string alias_name;
};
