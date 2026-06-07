# ChatServer 面向接口改造方案

## 目标

将 ChatServer 的核心依赖抽象为接口（C++ 纯虚类），使得业务代码依赖接口而非具体实现，从而在单元测试中可以用 Mock 对象替换真实组件。

## 设计原则

- **按调用方视角分类**：接口定义取决于"谁在调用、用来做什么"，而非"底层是什么技术"。
- **避免过度设计**：不为每个类都抽象接口，只为**被业务代码直接依赖、且测试时需要替换**的外部依赖创建接口。
- **保持现有目录结构**：接口定义放在 `src/domain/contract/` 下，实现类留在原位置。

## 需要抽象哪些接口

### 分类依据

按**依赖方向**划分：

1. **数据存储抽象**（Data Store）—— 业务代码读写数据时依赖的接口
2. **远程调用抽象**（RPC）—— 业务代码调用其他服务时依赖的接口
3. **基础设施抽象**（Infrastructure）—— 业务代码依赖的系统级能力（缓存、配置、心跳等）

---

### 1. 数据存储接口（Data Store）

#### 1.1 `IUserRepository` — 用户仓储

- **当前实现**：`UserDao`（MySQL）
- **调用方**：`UserCacheService`、`FriendDao::getFriendList`、`LoginHandler`
- **方法**：
  - `getUserInfo(int uid) -> shared_ptr<UserInfo>`
  - `getUserInfo(const string &name) -> shared_ptr<UserInfo>`

#### 1.2 `IFriendRepository` — 好友仓储

- **当前实现**：`FriendDao`（MySQL）
- **调用方**：`FriendHandler`、`LoginHandler`、`ChatServiceImpl`
- **方法**：
  - `addFriendApply(uid, touid, alias_name) -> bool`
  - `getApplyList(touid, list, begin, limit) -> bool`
  - `authFriendApply(uid, touid) -> bool`
  - `addFriend(applicant_uid, accepter_uid, alias1, alias2) -> bool`
  - `getFriendList(uid, list) -> bool`
  - `getFriendAlias(self_id, friend_id, out_alias) -> bool`
  - `getFriendApplyAlias(from_uid, to_uid, out_alias) -> bool`

#### 1.3 `IChatMessageRepository` — 聊天消息仓储

- **当前实现**：`ChatMessageDao`（MySQL）
- **调用方**：`ChatMessageHandler`、`ChatQueryHandler`、`PersistWorker`
- **方法**：
  - `saveMessage(msg, out_db_id) -> bool`
  - `existsByClientMsgId(from_uid, client_msg_id) -> bool`
  - `enqueueOffline(message_id, owner_uid) -> bool`
  - `fetchOfflineBatch(owner_uid, limit, out, out_inbox_ids) -> bool`
  - `removeOfflineByIds(inbox_ids) -> bool`
  - `queryHistory(self_uid, peer_uid, before_id, limit, out) -> bool`

---

### 2. 远程调用接口（RPC）

#### 2.1 `IStatusServiceClient` — StatusServer 客户端

- **当前实现**：`StatusGrpcClient`
- **调用方**：`FriendHandler`、`ChatMessageHandler`、`LoginHandler`、`ChatServer::main`
- **方法**：
  - `getUserChatNode(uid) -> optional<UserChatLocation>`
  - `bindUserToNode(uid, node_name) -> bool`
  - `registerChatNode(node) -> bool`
  - `unregisterChatNode(node) -> bool`
  - `heartbeatChatNode(name, instance_id) -> bool`

#### 2.2 `IChatServiceClient` — 跨节点 ChatServer 客户端

- **当前实现**：`ChatGrpcClient`
- **调用方**：`FriendHandler`、`ChatMessageHandler`
- **方法**：
  - `NotifyAddFriend(host, port, req) -> AddFriendRsp`
  - `NotifyAuthFriend(host, port, req) -> AuthFriendRsp`
  - `NotifyTextChatMsg(host, port, req, root_value) -> TextChatMsgRsp`

---

### 3. 基础设施接口（Infrastructure）

#### 3.1 `ICacheService` — 缓存服务

- **当前实现**：`RedisMgr`
- **调用方**：`UserCacheService`、`LoginHandler`、`ChatServer::main`
- **方法**：
  - `get(key, value) -> bool`
  - `set(key, value) -> bool`
  - `del(key) -> bool`
  - `hGet(key, field) -> string`
  - `hSet(key, field, value) -> bool`
  - `hDel(key, field) -> bool`
  - `hGetAll(key, out) -> bool`

#### 3.2 `IUserSessionManager` — 用户会话管理器

- **当前实现**：`UserMgr`
- **调用方**：`FriendHandler`、`ChatMessageHandler`、`ChatServiceImpl`、`LoginHandler`
- **方法**：
  - `getSession(uid) -> shared_ptr<CSession>`
  - `setUserSession(uid, session)`
  - `RemoveUserSession(uid)`

#### 3.3 `IUserCacheService` — 用户缓存查询服务

- **当前实现**：`UserCacheService`（纯静态类，内部依赖 `RedisMgr` + `UserDao`）
- **调用方**：`FriendHandler`、`ChatServiceImpl`、`LoginHandler`、`UserHandler`
- **说明**：`UserCacheService` 当前为纯静态类，无法通过继承实现多态。接口 `IUserCacheService` 保留定义，供将来重构为非静态类后使用，或直接用于 Mock 框架的 duck-typing。
- **方法**：
  - `getByUid(uid, user_info) -> bool`
  - `getByName(name, user_info) -> bool`
  - `fillSearchResultByUid(uid_str, result)`
  - `fillSearchResultByName(name_str, result)`

---

## 不需要抽象的内容

| 组件 | 原因 |
|------|------|
| `LogicSystem` | 消息路由分发逻辑，测试时可直接构造真实对象 |
| `Handler` 类（`LoginHandler`、`FriendHandler` 等） | 它们是编排层，测试时通过 Mock 下层接口来测 |
| `PersistWorker` | 异步队列工作者，可通过 Mock `IChatMessageRepository` 来隔离 |
| `HeartBeatHandler` | 网络层心跳管理，与 TCP 会话强绑定，单独测意义不大 |
| `CServer` / `CSession` / `MsgNode` | 网络层，测试时可用 Mock Session 替代 |
| `ConfigMgr` | 配置读取，测试时直接构造配置即可 |
| `RuntimeContext` / `NodeHeartbeat` | 节点运行时上下文，测试时可 Mock 或直接构造 |

## 接口文件组织

```
src/domain/contract/
├── IUserRepository.h
├── IFriendRepository.h
├── IChatMessageRepository.h
├── IStatusServiceClient.h
├── IChatServiceClient.h
├── ICacheService.h
├── IUserSessionManager.h
└── IUserCacheService.h
```

## 后续实施步骤

1. 在 `src/domain/contract/` 下创建上述接口头文件
2. 让现有实现类继承对应接口
3. 将业务代码中对具体类的直接依赖改为依赖接口（通过构造函数或 setter 注入）
4. 为 `MySqlMgr` 等工厂类提供返回接口指针的方法
5. 编写单元测试时，使用 Mock 框架（如 Google Mock）实现接口并注入
