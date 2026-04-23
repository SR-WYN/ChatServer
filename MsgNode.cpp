#include "MsgNode.h"
#include "const.h"
#include <boost/asio.hpp>
#include <boost/asio/detail/socket_ops.hpp>
#include <cstring>
#include <iostream>

MsgNode::MsgNode(short max_len) : _cur_len(0), _total_len(max_len), _data(new char[max_len + 1]())
{
    _data[_total_len] = '\0';
}

MsgNode::~MsgNode()
{
    std::cout << "MsgNode destruct" << std::endl;
    delete[] _data;
}

void MsgNode::clear()
{
    ::memset(_data, 0, _total_len);
    _cur_len = 0;
}

char *MsgNode::getData()
{
    return _data;
}

void MsgNode::setCurLen(short cur_len)
{
    _cur_len = cur_len;
}

short MsgNode::getCurLen()
{
    return _cur_len;
}

short MsgNode::getTotalLen()
{
    return _total_len;
}

short RecvNode::getMsgId()
{
    return _msg_id;
}

RecvNode::RecvNode(short max_len, short msg_id) : MsgNode(max_len), _msg_id(msg_id)
{
}

SendNode::SendNode(const char *msg, short max_len, short msg_id) : MsgNode(max_len), _msg_id(msg_id)
{
    // 先发送id，转为网络字节序
    short msg_id_host = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
    memcpy(_data, &msg_id_host, HEAD_ID_LEN);
    // 再发送长度，转为网络字节序
    short man_len_host = boost::asio::detail::socket_ops::host_to_network_short(max_len);
    memcpy(_data + HEAD_ID_LEN, &man_len_host, HEAD_DATA_LEN);
    // 再发送数据
    memcpy(_data + HEAD_TOTAL_LEN, msg, _cur_len);
}