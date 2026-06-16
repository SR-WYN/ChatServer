#pragma once

enum ErrorCodes
{
    SUCCESS = 0,              // 成功
    ERROR_JSON = 1001,        // JSON 解析错误
    RPC_FAILED = 1002,        // RPC 请求错误
    VERIFY_EXPIRED = 1003,    // 验证码过期
    VERIFY_CODE_ERROR = 1004, // 验证码错误
    USER_EXIST = 1005,        // 用户已存在
    PASSWD_ERROR = 1006,      // 密码错误
    EMAIL_NOT_MATCH = 1007,   // 邮箱不匹配
    PASSWD_UP_FAILED = 1008,  // 密码更新失败
    PASSWD_INVALID = 1009,    // 密码无效
    PASSWD_NOT_MATCH = 1010,  // 密码不匹配
    UID_INVALID = 1011,       // 用户不存在
    TOKEN_INVALID = 1012,     // 令牌无效
};

// 单帧包体最大长度（与协议头 2 字节 body_len 一致，最大 32767）
inline constexpr int MAX_LENGTH = 32767;
// 头部总长度
inline constexpr int HEAD_TOTAL_LEN = 4;
// 头部id长度
inline constexpr int HEAD_ID_LEN = 2;
// 头部数据长度
inline constexpr int HEAD_DATA_LEN = 2;
// 最大接收队列长度
inline constexpr int MAX_RECVQUE = 10000;
// 最大发送队列长度
inline constexpr int MAX_SENDQUE = 1000;

enum MSG_IDS
{
    MSG_CHAT_LOGIN = 2001,               // 聊天登录
    MSG_CHAT_LOGIN_RSP = 2002,           // 聊天登录响应
    MSG_SEARCH_USER_REQ = 2003,          // 搜索用户请求
    MSG_SEARCH_USER_RSP = 2004,          // 搜索用户响应
    MSG_ADD_FRIEND_REQ = 2005,           // 添加好友请求
    MSG_ADD_FRIEND_RSP = 2006,           // 添加好友响应
    MSG_NOTIFY_ADDFRIEND_REQ = 2007,     // 通知添加好友请求
    MSG_AUTH_FRIEND_REQ = 2008,          // 认证好友请求
    MSG_AUTH_FRIEND_RSP = 2009,          // 认证好友响应
    MSG_NOTIFY_AUTH_FRIEND_REQ = 2010,   // 通知认证好友请求
    MSG_TEXT_CHAT_MSG_REQ = 2011,        // 文本聊天消息请求
    MSG_TEXT_CHAT_MSG_RSP = 2012,        // 文本聊天消息响应
    MSG_NOTIFY_TEXT_CHAT_MSG_REQ = 2013, // 通知文本聊天消息请求
    MSG_CHAT_HISTORY_REQ = 2014,         // 聊天历史请求
    MSG_CHAT_HISTORY_RSP = 2015,         // 聊天历史响应
    MSG_HEARTBEAT_PING = 3001,           // 心跳 ping
    MSG_HEARTBEAT_PONG = 3002,           // 心跳 pong
};

// 应用层无消息超过该秒数则关闭连接（建议 >= 客户端 3×Ping 间隔）
constexpr int SESSION_APP_IDLE_SECONDS = 120;
// 空闲连接扫描周期（秒）
constexpr int IDLE_SWEEP_INTERVAL_SECONDS = 10;

namespace RedisPrefix
{
constexpr const char *CODE = "code_";
constexpr const char *USERTOKENPREFIX = "utoken_";
constexpr const char *IPCOUNTPREFIX = "ipcount_";
constexpr const char *LOGIN_COUNT = "logincount";
} // namespace RedisPrefix
