#pragma once

class MsgNode
{
public:
    // total_len 表示完整数据包长度（包头 + 包体）。
    MsgNode(short total_len);
    ~MsgNode();
    void clear();
    char *getData();
    void setCurLen(short cur_len);
    short getCurLen();
    short getTotalLen();

protected:
    short _cur_len;
    short _total_len;
    char *_data;
};

class RecvNode : public MsgNode
{
public:
    RecvNode(short body_len, short msg_id);
    short getMsgId();
private:
    short _msg_id;
};

class SendNode : public MsgNode
{
public:
    // 按协议组包：[2B msg_id][2B body_len][body]。
    SendNode(const char *body, short body_len, short msg_id);
private:
    short _msg_id;
};