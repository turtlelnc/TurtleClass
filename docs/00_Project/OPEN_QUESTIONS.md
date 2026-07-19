# Open Questions

仅记录尚未冻结、需要人工决策的问题。

## OQ-001：服务端传输协议

候选：

- HTTP
- WebSocket
- HTTP + WebSocket

状态：**已决定: HTTP/REST (Phase 2)**。

## OQ-002：正式存储实现

候选：

- 完全自定义追加式二进制日志
- 自定义事件文件 + SQLite 索引
- 其他方案

状态：**已决定: JSON Lines + SQLite Meta (Phase 2)**。

## OQ-003：WinUI 架构

候选：

- 自建轻量 MVVM
- Windows Community Toolkit MVVM
- 其他

状态：**已决定: WinUI 3 + MVVM (C++) (Phase 3)**。

## OQ-004：管理员界面

候选：

- 与日常客户端同程序
- 单独管理员程序
- 同程序但独立权限入口

状态：**已决定: 同程序但独立权限入口 (Phase 3)**。


## OQ-2026-07-17-WINUI-SCAFFOLD-BEFORE-API-FREEZE

- 状态：**已关闭** - API v1 已冻结，Phase 3 前置条件满足。
- 背景：用户明确要求继续编写 WinUI 端，但 Phase 3 原前置条件要求 API v1 与同步协议 v1 冻结。
- 临时处理：仅创建 Windows-only C++ desktop UI shell scaffold，不接入正式同步协议，不复制 Core 业务计算，不声明 Phase 3 完成。
- 后续决策：在 API/同步协议冻结前，是否允许继续扩展 C++ 桌面 UI 视觉与本地命令队列原型？
- 结果：允许，现已进入 Phase 3 完整开发。


## OQ-2026-07-17-WINDOWS-UI-TECH-CHOICE

- 状态：**已决定: WinUI 3 (Native C++)**。
- 背景：用户澄清 Windows UI 端应为 C++ 桌面应用 UI，不是基于 C# 的 WinUI 项目。
- 临时处理：将原 WinUI 3 XAML scaffold 替换为 native C++ Win32 desktop scaffold，目录为 `src/winui/TurtleClass.WindowsDesktop`。
- 后续决策：是否继续使用 Win32 原生控件，还是在 C++ 桌面框架中引入 WinUI 3/C++、WIL 或其他成熟 Windows UI 辅助库？
- 最终决定：采用 WinUI 3 with C++/WinRT，提供现代化 UI 体验同时保持原生性能。
