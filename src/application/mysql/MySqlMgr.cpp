#include "MySqlMgr.h"
#include "MySqlPool.h"

MySqlMgr::MySqlMgr()
{
    MySqlPool::initOnce();
}

MySqlMgr::~MySqlMgr() = default;

IUserRepository &MySqlMgr::users()
{
    return _user_dao;
}

IFriendRepository &MySqlMgr::friends()
{
    return _friend_dao;
}

IChatMessageRepository &MySqlMgr::chatMessages()
{
    return _chat_message_dao;
}
