#include "ChatServiceImpl.h"

ChatServiceImpl::ChatServiceImpl()
{
}

Status ChatServiceImpl::NotifyAddFriend(ServerContext* context,const AddFriendReq* request,AddFriendRsp* reply)
{
    //todo
}

Status ChatServiceImpl::NotifyAuthFriend(ServerContext* context,const AuthFriendReq* request,AuthFriendRsp* reply)
{
    //todo
}

Status ChatServiceImpl::NotifyTextChatMsg(ServerContext* context,const TextChatMsgReq* request,TextChatMsgRsp* reply)
{
    //todo
}

bool ChatServiceImpl::getBaseInfo(std::string base_key,int uid,std::shared_ptr<UserInfo>& user_info)
{
    //todo
}