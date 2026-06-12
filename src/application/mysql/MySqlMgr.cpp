#include "MySqlMgr.h"
#include "MySqlPool.h"

MySqlMgr::MySqlMgr()
{
    MySqlPool::initOnce();
}

MySqlMgr::~MySqlMgr() = default;

UserRepository &MySqlMgr::users()
{
    return _user_dao;
}

FriendRepository &MySqlMgr::friends()
{
    return _friend_dao;
}

ChatMessageRepository &MySqlMgr::chatMessages()
{
    return _chat_message_dao;
}
