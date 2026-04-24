#pragma once

enum ErrorCodes
{
    SUCCESS = 0,              // 成功
    ERROR_JSON = 1001,        // JSON 解析错误
    RPCFAILED = 1002,         // RPC 请求错误
    VERIFY_EXPIRED = 1003,    // 验证码过期
    VERIFY_CODE_ERROR = 1004, // 验证码错误
    USER_EXIST = 1005,        // 用户已存在
    PASSWD_ERROR = 1006,      // 密码错误
    EMAIL_NOT_MATCH = 1007,   // 邮箱不匹配
    PASSWD_UP_FAILED = 1008,  // 密码更新失败
    PASSWD_INVALID = 1009,    // 密码无效
    PASSWD_NOT_MATCH = 1010,  // 密码不匹配
    UID_INVALID = 1011,    // 用户不存在
    TOKEN_INVALID = 1012,    // 令牌无效
};

// 最大长度
#define MAX_LENGTH 1024 * 2
// 头部总长度
#define HEAD_TOTAL_LEN 4
// 头部id长度
#define HEAD_ID_LEN 2
// 头部数据长度
#define HEAD_DATA_LEN 2
// 最大接收队列长度
#define MAX_RECVQUE 10000
// 最大发送队列长度
#define MAX_SENDQUE 1000

enum MSG_IDS
{
    MSG_CHAT_LOGIN = 2001, // 聊天登录
    MSG_CHAT_LOGIN_RSP = 2002, // 聊天登录响应
};
