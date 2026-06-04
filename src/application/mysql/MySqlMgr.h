#pragma once
#include "ChatMessageDao.h"
#include "FriendDao.h"
#include "Singleton.h"
#include "UserDao.h"

class MySqlMgr : public Singleton<MySqlMgr>
{
    friend class Singleton<MySqlMgr>;

public:
    ~MySqlMgr() override;
    UserDao &users();
    FriendDao &friends();
    ChatMessageDao &chatMessages();

private:
    MySqlMgr();
    MySqlMgr(const MySqlMgr &) = delete;
    MySqlMgr &operator=(const MySqlMgr &) = delete;
    UserDao _user_dao;
    FriendDao _friend_dao;
    ChatMessageDao _chat_message_dao;
};
