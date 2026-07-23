#pragma once

#include "error_codes.h"

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
    MSG_FILE_TRANSFER_REQ = 2016,         // 索要 FileServer 地址
    MSG_FILE_TRANSFER_RSP = 2017,         // 返回 FileServer 地址+token
    MSG_FILE_TRANSFER_DONE = 2018,        // 通知删除 token
    MSG_IMAGE_CHAT_MSG_REQ = 2019,        // 图片聊天消息请求
    MSG_IMAGE_CHAT_MSG_RSP = 2020,        // 图片聊天消息响应
    MSG_NOTIFY_IMAGE_CHAT_MSG_REQ = 2021, // 通知图片聊天消息
    MSG_KICK_NOTIFY = 2022,              // 被踢通知
    MSG_HEARTBEAT_PING = 3001,           // 心跳 ping
    MSG_HEARTBEAT_PONG = 3002,           // 心跳 pong
};

// 应用层无消息超过该秒数则关闭连接（建议 >= 客户端 3×Ping 间隔）
constexpr int SESSION_APP_IDLE_SECONDS = 90;
// 空闲连接扫描周期（秒）
constexpr int IDLE_SWEEP_INTERVAL_SECONDS = 10;


