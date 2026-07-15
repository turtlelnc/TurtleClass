# TurtleClass Handoff

## 当前状态

- 当前阶段：Phase 1，服务器核心
- 当前里程碑：M1，建立 Core 最小可验证版本
- 代码状态：已建立 C++23/CMake Core 最小可验证版本
- 文档状态：项目主规范已建立
- 阻塞问题：无
- 最近更新时间：2026-07-14

## 下一步

1. 阅读 `PROJECT_MASTER.md`。
2. 阅读 `docs/02_Phases/PHASE_1_SERVER_CORE.md`。
3. 扩展 Phase 1 Core 的持久化边界与更多领域场景。
4. 保持只实现 Core 与测试。
5. 每次修改后编译并运行全部测试。
6. 更新工程进度与本文件。

## 当前禁止事项

- 不开发 WinUI。
- 不开发完整服务端网络。
- 不配置 Cloudflare。
- 不调用 DeepSeek。
- 不冻结正式磁盘格式。
- 不自行改变积分、等级、徽章、撤销规则。

## 交接给新 AI 时

至少提供：

- `PROJECT_MASTER.md`
- `HANDOFF.md`
- `AGENTS.md`
- 当前阶段文档
- 最新 Git diff 或提交记录
