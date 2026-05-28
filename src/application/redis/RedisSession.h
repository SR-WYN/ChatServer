#pragma once

#include "RedisPool.h"
#include "utils.h"
#include <cstring>
#include <hiredis/hiredis.h>
#include <iostream>
#include <string>
#include <vector>

class RedisSession
{
public:
    template <typename Fn>
    static bool withConn(Fn &&fn)
    {
        auto &pool = RedisPool::getInstance();
        redisContext *ctx = pool.getConnection();
        if (!ctx)
        {
            return false;
        }
        utils::Defer defer([&]() { pool.returnConnection(ctx); });
        return fn(ctx);
    }

    static bool get(const std::string &key, std::string &val)
    {
        val.clear();
        return withConn([&](redisContext *ctx) {
            redisReply *reply = execArgv(ctx, {"GET", key});
            if (!reply)
            {
                return false;
            }
            ReplyGuard guard(reply);
            if (reply->type != REDIS_REPLY_STRING)
            {
                return false;
            }
            val = reply->str;
            return true;
        });
    }

    static bool set(const std::string &key, const std::string &val)
    {
        return withConn([&](redisContext *ctx) {
            redisReply *reply = execArgv(ctx, {"SET", key, val});
            if (!reply)
            {
                return false;
            }
            ReplyGuard guard(reply);
            return reply->type == REDIS_REPLY_STATUS &&
                   (strcmp(reply->str, "OK") == 0 || strcmp(reply->str, "ok") == 0);
        });
    }

    static std::string hGet(const std::string &key, const std::string &field)
    {
        std::string val;
        withConn([&](redisContext *ctx) {
            redisReply *reply = execArgv(ctx, {"HGET", key, field});
            if (!reply)
            {
                return false;
            }
            ReplyGuard guard(reply);
            if (reply->type == REDIS_REPLY_NIL)
            {
                return true;
            }
            if (reply->type == REDIS_REPLY_ERROR)
            {
                return false;
            }
            if (reply->type == REDIS_REPLY_STRING)
            {
                val = reply->str;
            }
            return true;
        });
        return val;
    }

    static bool hSet(const std::string &key, const std::string &field, const std::string &val)
    {
        return withConn([&](redisContext *ctx) {
            redisReply *reply = execArgv(ctx, {"HSET", key, field, val});
            if (!reply)
            {
                return false;
            }
            ReplyGuard guard(reply);
            return reply->type == REDIS_REPLY_INTEGER;
        });
    }

    static bool hDel(const std::string &key, const std::string &field)
    {
        return withConn([&](redisContext *ctx) {
            redisReply *reply = execArgv(ctx, {"HDEL", key, field});
            if (!reply)
            {
                return false;
            }
            ReplyGuard guard(reply);
            if (reply->type != REDIS_REPLY_INTEGER)
            {
                return false;
            }
            return reply->integer > 0;
        });
    }

private:
    struct ReplyGuard
    {
        redisReply *reply = nullptr;
        explicit ReplyGuard(redisReply *r) : reply(r) {}
        ~ReplyGuard()
        {
            if (reply)
            {
                freeReplyObject(reply);
            }
        }
        ReplyGuard(const ReplyGuard &) = delete;
        ReplyGuard &operator=(const ReplyGuard &) = delete;
    };

    static redisReply *execArgv(redisContext *ctx, const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            return nullptr;
        }
        std::vector<const char *> argv;
        std::vector<size_t> arglen;
        argv.reserve(args.size());
        arglen.reserve(args.size());
        for (const auto &arg : args)
        {
            argv.push_back(arg.c_str());
            arglen.push_back(arg.size());
        }
        redisReply *reply = static_cast<redisReply *>(
            redisCommandArgv(ctx, static_cast<int>(argv.size()), argv.data(), arglen.data()));
        if (!reply)
        {
        }
        return reply;
    }
};
