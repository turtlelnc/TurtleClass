# TurtleClass

TurtleClass 是一个仅供单个班级使用、计划开源的班级积分与互动系统。

项目采用：

- C++23
- 离线优先
- 事件驱动
- 多设备同步
- 可审计、可重建、可恢复
- macOS 服务端
- Windows WinUI 3 客户端
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

1. 服务器核心：领域模型、事件、重放、存储基础
2. 服务器端：认证、同步、备份、冲突、维护与审查
3. WinUI：离线客户端、快速操作、同步与管理员入口

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
g++ -std=c++23 -Iinclude src/core/domain.cpp tests/core_tests.cpp -o turtleclass_core_tests
./turtleclass_core_tests
```
