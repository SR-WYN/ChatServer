#include "MySqlMgr.h"
#include "MySqlPool.h"

MySqlMgr::MySqlMgr()
{
    MySqlPool::initOnce();
}

MySqlMgr::~MySqlMgr() = default;

UserDao &MySqlMgr::users()
{
    return _user_dao;
}

FriendDao &MySqlMgr::friends()
{
    return _friend_dao;
}

ChatMessageDao &MySqlMgr::chatMessages()
{
    return _chat_message_dao;
}
