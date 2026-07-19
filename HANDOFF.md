# TurtleClass Handoff

## 当前状态

- 当前阶段：**Phase 3 - Windows Client Alpha** (Phase 2 已完成)
- 当前里程碑：**M3 - Windows 客户端登录、认证与基础 UI**
- 代码状态：Core 100% 完成，Server 85% 完成，Windows UI 40% 完成
- 文档状态：所有架构决策已冻结，RFC 001-006 已创建
- 阻塞问题：无
- 最近更新时间：2026-07-19

## Phase 2 完成情况

Phase 2 已于 2026-07-19 完成，主要成果：

### 服务器端（85% 完成）
- [x] HTTP/REST API v1 框架（RFC-001）
- [x] 账号系统（班级账号 + 管理员授权）
- [x] 设备注册表（内存实现）
- [x] Ed25519 设备认证（RFC-005）
- [x] JWT 令牌生成与验证
- [x] 事件上传/下载 API
- [x] 快照系统框架（RFC-006）
- [x] 冲突检测框架
- [x] 健康检查接口 `/health`
- [x] API 版本化 `/api/v1/`（RFC-006）
- [ ] 持久化设备表（待实现：SQLite 集成）
- [ ] 完整冲突解决流程（待实现）
- [ ] CLI 同步模拟器（待实现）

### 存储格式（RFC-002）
- [x] JSON Lines 事件日志格式定义
- [x] SQLite 元数据索引设计
- [ ] 完整实现（待 Phase 2 后期）

### 安全（RFC-004, RFC-005）
- [x] Ed25519 密钥对生成
- [x] 事件签名验证
- [x] JWT 设备令牌
- [ ] 重放攻击防护完整实现（待完善）

## Phase 3 进展（进行中）

### 已完成（40%）
- [x] WinUI 3 + C++/WinRT 架构决策（RFC-003）
- [x] MVVM 模式采用
- [x] AuthenticationService 实现
  - 登录接口（班级代码 + 学生 ID + 密码）
  - JWT 令牌管理
  - 登出功能
  - 登录状态回调
- [x] MainWindow 更新
  - 登录表单（班级代码、学生 ID、密码输入框）
  - 登录/登出按钮
  - 基于登录状态的 UI 启用/禁用
  - 学生列表显示（需登录后访问）
  - 快速加减分队列（需登录后使用）
- [x] 资源头文件更新（新控件 ID）
- [x] README 文档更新

### 待实现（60%）
- [ ] 真实 HTTP API 调用（替换 placeholder）
- [ ] 本地 SQLite 存储（离线事件队列持久化）
- [ ] 同步协议 v1 客户端实现
- [ ] 自动同步机制
- [ ] 冲突提示 UI
- [ ] 撤销操作 UI
- [ ] 管理员模式入口
- [ ] 安装/升级流程
- [ ] 低性能电脑测试

## 下一步（Phase 3 优先级）

1. **立即**: 将 AuthenticationService 的 placeholder 登录替换为真实 HTTP API 调用
2. **高优先级**: 实现本地 SQLite 存储用于离线事件队列持久化
3. **高优先级**: 实现同步协议 v1 客户端（事件上传/下载）
4. **中优先级**: 添加自动同步触发器（网络变化、定时、启动时）
5. **中优先级**: 实现冲突检测 UI 提示
6. **中优先级**: 添加撤销操作 UI
7. **低优先级**: 管理员模式切换和界面
8. **低优先级**: 安装程序制作（MSI/MSIX）

## 关键架构决定（已全部冻结）

| 决定 | 方案 | RFC | 状态 |
|---|---|---|---|
| 网络协议 | HTTP/HTTPS | RFC-001 | ✅ 已冻结 |
| 存储格式 | JSON Lines + SQLite Meta | RFC-002 | ✅ 已冻结 |
| 管理员界面 | 同程序独立权限入口 | RFC-003 | ✅ 已冻结 |
| WinUI 架构 | WinUI 3 + MVVM (C++) | RFC-003 | ✅ 已冻结 |
| 设备凭证 | Ed25519 密钥对 + JWT | RFC-004 | ✅ 已冻结 |
| API 版本化 | URL 路径 /api/v1/ | RFC-005 | ✅ 已冻结 |
| 冲突/快照 | 局部冻结 + 管理员解决 | RFC-006 | ✅ 已冻结 |

## 工程结构

```
/workspace
├── include/TurtleClass/
│   ├── Core/           # 领域模型、状态投影、规则引擎
│   ├── Security/       # Ed25519、JWT、设备认证
│   └── Server/         # HTTP API、账号、快照
├── src/
│   ├── core/           # Core 实现
│   ├── security/       # Security 实现
│   ├── server/         # Server 实现
│   └── winui/          # Windows 客户端
│       └── TurtleClass.WindowsDesktop/
│           ├── AuthenticationService.*  # 认证服务
│           ├── MainWindow.*             # 主窗口
│           ├── StudentListViewModel.*   # 学生列表 VM
│           └── resource.h               # 资源 ID
├── tests/              # 单元测试
├── docs/
│   ├── 00_Project/     # 项目文档
│   ├── 02_Phases/      # 阶段文档
│   └── 05_RFC/         # 架构决策记录（RFC 001-006）
└── build/              # 构建输出
```

## 交接给新 AI 时

至少提供：

- `PROJECT_MASTER.md`（唯一事实来源）
- `HANDOFF.md`（本文件）
- `AGENTS.md`
- 当前阶段文档 `docs/02_Phases/PHASE_3_WINUI.md`
- 最新 Git diff 或提交记录
- 编译并运行测试验证当前状态

## 快速验证命令

```bash
cd /workspace
mkdir -p build && cd build
cmake ..
make -j4
./TurtleClassTests
# 预期输出：All TurtleClass core tests passed / All TurtleClass server tests passed

# Windows 客户端需要在 Windows 上构建：
# 打开 src/winui/TurtleClass.WindowsDesktop/TurtleClass.WindowsDesktop.vcxproj
# 在 Visual Studio 2022 中构建
```

## Git 提交历史摘要

- `7780cd3` Phase 3: Add Windows client login and authentication
- `8a60029` Phase 2: Add HTTP server API skeleton
- `c3abf89` Fix account tests
- `6e5b5a7` Merge pull request #3 from turtlelnc/work

## 已知问题与技术债务

1. **AuthenticationService**: 当前使用 placeholder 登录逻辑，需要替换为真实 HTTP API 调用
2. **设备表持久化**: 服务器设备表仍在内存中，重启后丢失
3. **冲突解决**: 框架已搭建但完整流程未实现
4. **CLI 模拟器**: 尚未创建，无法测试双设备离线同步场景
5. **日志保留**: 策略已定义但未实现自动清理
6. **备份触发**: 策略已定义但未实现自动备份

## 禁止事项

- 不改变已冻结的架构决策（RFC 001-006）
- 不在 UI 层计算积分、等级、徽章（必须通过 Core）
- 不使用 SQLite 于服务器端 Phase 2（保持简单文件存储）
- 不开放正式服务端公网网络（保持 localhost 测试直到 API 完全测试）
