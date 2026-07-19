# TurtleClass Handoff

## 当前状态

- 当前阶段：Phase 2，服务器端 alpha
- 当前里程碑：M1，建立 Core 最小可验证版本
- 代码状态：已建立 Core 校验加强版、本地服务器后端 alpha、设备注册/撤销校验与 Windows C++ 桌面 UI shell scaffold
- 文档状态：项目主规范已建立
- 阻塞问题：无
- 最近更新时间：2026-07-17

## 下一步

1. 阅读 `PROJECT_MASTER.md`。
2. 阅读 `docs/02_Phases/PHASE_1_SERVER_CORE.md`。
3. 扩展 Phase 2 的班级账号、管理员密码授权、持久化设备表、冲突管理与协议冻结前测试；Windows C++ 桌面 UI 仅保持 shell，不做正式同步集成。
4. 暂不开放正式公网 API，不配置 Cloudflare，不接入 DeepSeek 自动审查。
5. 每次修改后编译并运行全部测试。
6. 更新工程进度与本文件。

## 当前禁止事项

- 不进行正式 Windows UI 同步/API 集成，除非协议冻结或明确要求。
- 不开放正式服务端公网网络。
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
