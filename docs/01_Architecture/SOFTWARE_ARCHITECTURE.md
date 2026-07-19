# Software Architecture

## 1. 架构目标

- Core 独立
- 状态可重建
- 客户端离线可用
- 服务端可验证客户端事件
- 冲突局部化
- AI 与正式业务隔离

## 2. 逻辑架构

```text
WinUI Client
 ├─ Presentation
 ├─ Local Projection
 ├─ Local Event Queue
 └─ Sync Client
          │
          ▼
TurtleClass Server
 ├─ Authentication
 ├─ Device Registry
 ├─ Sync Coordinator
 ├─ Event Validator
 ├─ Conflict Manager
 ├─ Snapshot Manager
 ├─ Backup Manager
 └─ Audit Dispatcher
          │
          ▼
Shared Libraries
 ├─ Core
 ├─ Storage
 ├─ Security
 └─ Sync Protocol
```

## 3. 依赖方向

```text
Core
↑
Storage / Security / Sync
↑
Server / Client
↑
WinUI
```

Core 不得依赖上层。

## 4. 事件流程

```text
用户操作
→ 创建命令
→ Core 验证
→ 生成事件组
→ 本地追加
→ 本地投影
→ 上传服务端
→ 服务端再次验证
→ 分配全局序号
→ 其他客户端下载
→ 重放
```

## 5. 失败处理

- 本地追加失败：不更新 UI 为成功
- 服务端拒绝：保留本地记录并显示错误
- 网络失败：保留待同步队列
- 快照损坏：从事件重建
- 事件链损坏：停止受影响范围并报告
- AI 失败：仅记录审查不可用
