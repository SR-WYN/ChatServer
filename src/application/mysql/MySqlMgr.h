#pragma once
#include "ChatMessageDao.h"
#include "ChatMessageRepository.h"
#include "FriendDao.h"
#include "FriendRepository.h"
#include "Singleton.h"
#include "UserDao.h"
#include "UserRepository.h"

class MySqlMgr : public Singleton<MySqlMgr>
{
    friend class Singleton<MySqlMgr>;

public:
    ~MySqlMgr() override;
    UserRepository &users();
    FriendRepository &friends();
    ChatMessageRepository &chatMessages();

private:
    MySqlMgr();
    MySqlMgr(const MySqlMgr &) = delete;
    MySqlMgr &operator=(const MySqlMgr &) = delete;
    UserDao _user_dao;
    FriendDao _friend_dao;
    ChatMessageDao _chat_message_dao;
};
