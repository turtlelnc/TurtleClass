# Open Questions

仅记录尚未冻结、需要人工决策的问题。

## OQ-001：服务端传输协议

候选：

- HTTP
- WebSocket
- HTTP + WebSocket

状态：未决定。

## OQ-002：正式存储实现

候选：

- 完全自定义追加式二进制日志
- 自定义事件文件 + SQLite 索引
- 其他方案

状态：不得在 Phase 1 冻结。

## OQ-003：WinUI 架构

候选：

- 自建轻量 MVVM
- Windows Community Toolkit MVVM
- 其他

状态：Phase 3 前决定。

## OQ-004：管理员界面

候选：

- 与日常客户端同程序
- 单独管理员程序
- 同程序但独立权限入口

状态：未决定。
