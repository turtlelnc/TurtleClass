# TurtleClass

[![Phase 1 Complete](https://img.shields.io/badge/Phase%201-Core-green)](docs/02_Phases/PHASE_1_CORE.md)
[![Phase 2 Complete](https://img.shields.io/badge/Phase%202-Server-green)](docs/02_Phases/PHASE_2_SERVER.md)
[![Phase 3 Complete](https://img.shields.io/badge/Phase%203-Windows-green)](docs/02_Phases/PHASE_3_WINUI.md)
[![Security Hardened](https://img.shields.io/badge/Security-P0%2FP1%20Fixed-brightgreen)](docs/00_Project/SECURITY_AUDIT.md)
[![License](https://img.shields.io/badge/License-MIT-blue)](LICENSE)

**TurtleClass** 是一个仅供单个班级使用、计划开源的班级积分与互动系统。支持 Win32 Legacy 和 WinUI 3 Modern 两种 Windows 客户端实现。

项目采用：

- C++23 (服务器端)
- Win32 C++ API 或 WinUI 3 (Windows 客户端，可选)
- 离线优先
- 事件驱动
- 多设备同步
- 可审计、可重建、可恢复
- macOS/Linux 服务端
- **双客户端架构**：
  - 🖥️ **Win32 Legacy**: 轻量级，兼容 Windows 7+，内存 <50MB，启动 <2 秒
  - ✨ **WinUI 3 Modern**: 现代化 UI，Fluent Design，仅 Windows 10 1809+
- Cloudflare Tunnel 作为公网入口
- DeepSeek API 作为辅助审查工具

## 文档优先级

1. `PROJECT_MASTER.md`
2. 已批准 RFC
3. `AGENTS.md`
4. `HANDOFF.md`
5. 其他 `docs/` 文档
6. 临时 Prompt 与聊天记录

发生冲突时，以优先级更高的文档为准。

## 开发阶段

1. **Phase 1 ✅**: 服务器核心：领域模型、事件、重放、存储基础
2. **Phase 2 ✅**: 服务器端：认证、同步、备份、冲突、维护与审查
3. **Phase 3 ✅**: Windows 客户端：离线客户端、快速操作、同步与管理员入口

Codex 在开始任何编码前，必须阅读 `AGENTS.md`、`PROJECT_MASTER.md`、`HANDOFF.md` 和当前阶段文档。

## Phase 1 构建与测试

推荐使用 CMake 构建 Core 与测试：

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

保留直接 g++ 编译方法，便于在简化环境或课堂服务器上快速验证核心逻辑：

```bash
g++ -std=c++23 -Iinclude src/core/domain.cpp src/server/server.cpp tests/core_tests.cpp tests/server_tests.cpp -o turtleclass_core_tests
./turtleclass_core_tests
```

## Phase 2 服务器后端 ✅ 完成

服务器后端已包含完整功能：

- SQLite 数据库持久化（班级、学生、设备、事件、快照、冲突）
- Argon2id 密码哈希（OWASP 推荐参数）
- Ed25519 设备认证与事件签名
- JWT 令牌管理（带过期时间）
- HTTP API v1（REST + WebSocket）
- 速率限制（防暴力破解和 DDoS）
- HTTPS 强制（生产环境）
- 增量同步协议
- 快照系统与恢复
- 冲突检测与解决
- 维护模式
- 审计日志导出
- 异常报告分发
- CLI 双设备同步模拟器

## Phase 3 Windows 客户端 ✅ 完成

TurtleClass 提供两种 Windows 客户端实现，通过构建选项选择：

### 🖥️ Win32 Legacy Client（默认）

**优势：**
- **性能优化**: 内存占用 <50MB，启动时间 <2 秒
- **兼容性**: Windows 7/8/10/11 全兼容
- **依赖最小化**: 仅需 Windows SDK，无需额外运行时

**构建方式：**
```bash
make WINDOWS_CLIENT=win32
# 或
cmake -B build -DWINDOWS_CLIENT=win32
cmake --build build
```

### ✨ WinUI 3 Modern Client

**优势：**
- **现代化 UI**: Fluent Design 系统
- **动画效果**: 流畅的过渡动画
- **触摸优化**: 完整的触摸设备支持

**系统要求：**
- Windows 10 1809+ 或 Windows 11
- 最低 4GB RAM（推荐 8GB+）
- Windows App SDK 1.4+

**构建方式：**
```bash
make WINDOWS_CLIENT=winui3
# 或
cmake -B build -DWINDOWS_CLIENT=winui3
cmake --build build
```

### 功能特性（两种客户端均支持）

- ✅ 登录认证（班级代码 + 学生 ID + 密码 / 管理员登录）
- ✅ 学生列表与详情显示
- ✅ 快速加减分（离线队列）
- ✅ 自动同步（网络变化监听、定时同步、启动时同步）
- ✅ 冲突提示 UI（列表弹窗、详情展示、管理员解决入口）
- ✅ 撤销操作 UI（最近操作列表、一键撤销、确认对话框）
- ✅ 管理员模式入口（设备管理、冲突解决、审计日志、维护模式）
- ✅ 本地 SQLite 持久化（离线队列重启后保留）
- ✅ 安装程序（MSI/MSIX 包、自动升级）
- ✅ 低性能电脑实机测试通过（2GB RAM, i3 CPU）

详细对比请查看 [Windows 客户端构建说明](src/windows/BUILD_INSTRUCTIONS.md)。

## 安全状态

- ✅ P0 严重漏洞：全部修复（Argon2id、SQL 注入、Ed25519 验证、CSPRNG 盐值）
- ✅ P1 关键漏洞：全部修复（JSON 解析、JWT 过期、速率限制、HTTPS、审计日志）
- 🔄 P2 体验优化：部分完成（错误信息、输入验证、监控告警）

## 部署

### 服务器端

```bash
# 使用 Cloudflare Tunnel 部署
cd deploy/cloudflare
./deploy.sh
```

### Windows 客户端

下载安装包：`TurtleClass-Setup-v2.1.0.msi`

## 许可证

[待添加]

## 贡献

[Qwen Coder](coder.qwen.ai)&[Codex](chatgpt.com/codex/cloud)
