# TurtleClass Handoff

## 当前状态

- 当前阶段：Phase 2，服务器端 alpha - 账号系统与安全基础
- 当前里程碑：M2 - 账号系统、设备认证、冲突检测、快照与正式网络 API
- 代码状态：Core 100% 完成，Server alpha 35% 完成，Storage/Security/Sync 进行中
- 文档状态：PROJECT_MASTER.md 已同步更新（2026-07-19），Phase 1 正式验收
- 阻塞问题：无（架构决策已部分冻结）
- 最近更新时间：2026-07-19

## Phase 1 验收确认

Phase 1 已于 2026-07-19 正式验收完成：

- [x] C++23 + CMake 工程骨架
- [x] Core 领域模型（ID、规则、状态、事件、事件组）
- [x] 内存事件仓库
- [x] 状态投影器
- [x] 领域服务（提交、补偿撤销）
- [x] 一致性检查器
- [x] 单元测试覆盖所有核心场景
- [x] macOS 构建通过
- [x] 文档同步更新

## 下一步（Phase 2 优先级）

1. ** RFC-001**: 冻结 HTTP/HTTPS 作为正式网络 API 协议
2. ** RFC-002**: 冻结 TSV 追加式日志 + 快照文件格式
3. ** RFC-005**: 冻结 Ed25519 设备凭证格式
4. ** RFC-006**: 冻结 URL 路径版本化 /api/v1/
5. 实现班级账号系统（AccountId、班级登录）
6. 实现管理员账号及 Argon2id 密码授权
7. 持久化设备表（当前为内存注册表）
8. 实现正式设备认证（Ed25519 签名验证）
9. 实现事件签名与重放攻击防护
10. 实现快照系统与损坏恢复
11. 实现冲突检测与局部资产冻结
12. 实现管理员冲突处理流程
13. 创建 CLI 客户端模拟器（双设备离线同步场景测试）
14. 实现正式网络 API（HTTP/HTTPS，非本地接口）
15. 添加健康检查接口（/health）
16. 完善日志导出功能
17. 冻结 API v1 和同步协议 v1
18. 接入 DeepSeek 每日只读审查（API 冻结后）
19. Cloudflare Tunnel 部署准备（API 冻结后）

## 当前禁止事项

- 不在 API/协议冻结前扩展 Windows UI 正式同步集成
- 不开放正式服务端公网网络（保持 localhost 测试）
- 不配置 Cloudflare（等待 API 冻结）
- 不调用 DeepSeek（等待 API 冻结）
- 不自行改变积分、等级、徽章、撤销规则
- 不使用 SQLite（Phase 2 保持简单文件存储）

## 关键架构决定（已冻结）

| 决定 | 方案 | RFC |
|---|---|---|
| 网络协议 | HTTP/HTTPS | RFC-001（待创建） |
| 存储格式 | TSV 追加式日志 + 快照 | RFC-002（待创建） |
| SQLite 使用 | Phase 2 不使用 | - |
| 管理员界面 | 同程序独立权限入口 | RFC-004（待创建） |
| 设备凭证 | Ed25519 密钥对 | RFC-005（待创建） |
| API 版本化 | URL 路径 /api/v1/ | RFC-006（待创建） |

## 交接给新 AI 时

至少提供：

- `PROJECT_MASTER.md`（唯一事实来源）
- `HANDOFF.md`（本文件）
- `AGENTS.md`
- 当前阶段文档 `docs/02_Phases/PHASE_2_SERVER.md`
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
```
