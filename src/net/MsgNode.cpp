#include "MsgNode.h"
#include "const.h"
#include <boost/asio.hpp>
#include <boost/asio/detail/socket_ops.hpp>
#include <cstring>
#include <iostream>

MsgNode::MsgNode(short total_len)
    : _cur_len(0), _total_len(total_len), _data(new char[total_len + 1]())
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

RecvNode::RecvNode(short body_len, short msg_id) : MsgNode(body_len), _msg_id(msg_id)
{
}

SendNode::SendNode(const char *body, short body_len, short msg_id)
    : MsgNode(static_cast<short>(HEAD_TOTAL_LEN + body_len)), _msg_id(msg_id)
{
    // 包头由消息 ID 和包体长度组成，均使用网络字节序。
    short msg_id_net = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
    short body_len_net = boost::asio::detail::socket_ops::host_to_network_short(body_len);
    memcpy(_data, &msg_id_net, HEAD_ID_LEN);
    memcpy(_data + HEAD_ID_LEN, &body_len_net, HEAD_DATA_LEN);

    // 包体紧跟固定长度包头之后。
    memcpy(_data + HEAD_TOTAL_LEN, body, body_len);
    _cur_len = _total_len;
}