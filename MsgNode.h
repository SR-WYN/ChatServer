#pragma once

class MsgNode
{
public:
    MsgNode(short max_len);
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
    RecvNode(short max_len, short msg_id);
    short getMsgId();
private:
    short _msg_id;
};

class SendNode : public MsgNode
{
public:
    SendNode(const char *msg, short max_len, short msg_id);
private:
    short _msg_id;
};