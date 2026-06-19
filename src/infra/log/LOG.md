# ChatServer 日志使用说明

## 1. 日志写到哪里

`config.json` 中 `Log.Dir` 为日志目录的**绝对路径**（各环境自行修改），所有 `.log` 直接放在该目录下，例如：

`/home/asus/NETLEARN/ChatServer/src/logdir/app.log`

| 枚举 `LogModule` | 文件名 | 用途 |
|------------------|--------|------|
| `App` | `app.log` | 进程启动、退出、未捕获异常（`app/ChatServer.cpp`） |
| `Config` | `config.log` | 配置加载、节点选槽、运行时配置、向 Status 注册心跳（`infra/config/`） |
| `Net` | `net.log` | TCP 监听、accept、会话读写（`net/CServer`、`CSession`、`MsgNode`、`AsioIOServicePool`） |
| `Mysql` | `mysql.log` | 连接池、DAO、SQL 异常（`application/mysql/`） |
| `Redis` | `redis.log` | 连接池、命令失败（`application/redis/`） |
| `Grpc` | `grpc.log` | 对 Gate/Status/Chat 的 gRPC 客户端及 `ChatServiceImpl`（`infra/grpc/`） |
| `Logic` | `logic.log` | TCP 消息 ID 分发、未注册回调（`application/LogicSystem`） |
| `Login` | `login.log` | 登录鉴权（`application/login/LoginHandler`） |
| `User` | `user.log` | 用户缓存、在线管理、空闲断连（`application/user/`） |
| `Friend` | `friend.log` | 加好友、同意好友（`application/friend/`） |
| `Chat` | `chat.log` | 发消息、投递、落库、离线队列、历史（`application/chat/`） |

只有**实际打过日志**的模块才会生成对应文件。

---

## 2. 配置 `config.json`

```json
"Log": {
  "Dir": "/home/asus/NETLEARN/ChatServer/src/logdir",
  "Level": "info"
}
```

`Level`：`trace` / `debug` / `info` / `warn` / `error` / `critical` / `off`（低于该级别的日志不会写入）。

---

## 3. 初始化（`main` 里，且必须在 `ConfigMgr` 之后）

```cpp
#include "ConfigMgr.h"
#include "Log.h"

ConfigMgr::getInstance();
if (!Log::init("ChatServer", ConfigMgr::getInstance().getLogConfig())) {
    return 1;
}

// 进程正常或异常退出前
Log::shutdown();
```

`Log::init` 失败表示无法创建 `Log.Dir` 目录，应直接退出。

---

## 4. 在业务代码里打日志

```cpp
#include "Log.h"

// 必须使用 LogModule 枚举，不能传字符串
Log::info(LogModule::Net, "TCP listening on port {}", port);
Log::error(LogModule::Mysql, "saveMessage failed msgid={}", msgid);

// 宏：第一个参数同样是 LogModule
LOGW(LogModule::Redis, "command failed: {}", err);
LOGE(LogModule::Chat, "deliver failed from={} to={}", from, to);
```

### 4.1 级别

| API | 宏 |
|-----|-----|
| `Log::trace` | `LOGT` |
| `Log::debug` | `LOGD` |
| `Log::info` | `LOGI` |
| `Log::warn` | `LOGW` |
| `Log::error` | `LOGE` |
| `Log::critical` | `LOGC` |

### 4.2 格式

- 使用 fmt 风格占位符：`{}`，不要用 `%s`。
- 示例：`Log::info(LogModule::User, "uid={} online", uid);`

### 4.3 新增模块

新增模块：在 [`LogModule.h`](LogModule.h) 里按顺序增加 `LogModule` 枚举项、`LogNames` 里同名 `_snake_case` 字符串常量，并加入 `LogNames::_table`；**不要**在业务里写字符串模块名。

命名约定：成员变量 `_snake_case`；成员函数小写驼峰（`init`、`createModuleLogger`、`moduleName`）。

## 文件说明

| 文件 | 作用 |
|------|------|
| `LogModule.h` | 模块枚举 + `LogNames` 常量 + `moduleName()` |
| `Log.h` / `Log.cpp` | 写日志 API 与文件 sink |
| `LOG.md` | 本文档 |

配置读取在 `infra/config/ConfigMgr` 的 `getLogConfig()`，无单独 Loader 头文件。

---

## 5. 按目录选哪个模块（速查）

| 代码位置 | 使用 |
|----------|------|
| `src/app/` | `LogModule::App` |
| `src/infra/config/` | `LogModule::Config` |
| `src/net/` | `LogModule::Net` |
| `src/infra/grpc/`、`ChatServiceImpl` | `LogModule::Grpc` |
| `src/application/LogicSystem.*` | `LogModule::Logic` |
| `src/application/login/` | `LogModule::Login` |
| `src/application/user/` | `LogModule::User` |
| `src/application/friend/` | `LogModule::Friend` |
| `src/application/chat/` | `LogModule::Chat` |
| `src/application/mysql/` | `LogModule::Mysql` |
| `src/application/redis/` | `LogModule::Redis` |

---

## 6. 注意

- 须在 `Log::init` **成功之后**再调用 `Log::info` 等。
- 从 `build/` 运行时要保证同目录有 `config.json`。
- 日志仅写文件，不输出控制台；单文件持续追加，需自行清理旧日志。
