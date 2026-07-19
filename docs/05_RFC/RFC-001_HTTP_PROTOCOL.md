# RFC-001：HTTP/HTTPS 网络协议

## 状态

Accepted

## 背景

Phase 2 需要正式的网络 API 用于客户端与服务器通信。当前服务器仅支持本地接口调用，需要定义正式的网络协议。

## 问题

选择何种网络协议用于 TurtleClass 客户端 - 服务器通信？

## 目标

- 简单、成熟、易实现的协议
- 支持请求/响应模式
- 支持流式下载
- 便于调试和测试
- 可通过 Cloudflare Tunnel 暴露

## 非目标

- 实时双向通信（WebSocket）
- 高性能流媒体传输
- 复杂的 RPC 框架

## 提案

**决定：使用 HTTP/HTTPS 作为唯一网络协议。**

### API 设计原则

1. RESTful 风格资源路径
2. JSON 请求/响应体
3. 标准 HTTP 状态码
4. URL 路径版本化（/api/v1/）
5. 无状态请求（认证信息包含在每个请求中）

### 端点列表

| 方法 | 路径 | 描述 |
|---|---|---|
| GET | /health | 健康检查 |
| POST | /api/v1/auth/login | 班级/管理员登录 |
| POST | /api/v1/devices/register | 设备注册 |
| POST | /api/v1/devices/revoke | 设备撤销 |
| POST | /api/v1/events/upload | 上传事件组 |
| GET | /api/v1/events/download?after={seq} | 增量下载事件 |
| GET | /api/v1/snapshot | 获取最新快照 |
| POST | /api/v1/snapshot | 上传快照（管理员） |
| GET | /api/v1/conflicts | 列出冲突（管理员） |
| POST | /api/v1/conflicts/{id}/resolve | 解决冲突（管理员） |
| GET | /api/v1/export/events | 导出事件日志（管理员） |
| GET | /api/v1/export/logs | 导出完整日志（管理员） |
| GET | /api/v1/audit/reports | 获取审查报告（管理员） |

### 认证机制

- 班级账号：`X-Class-Id` + `X-Class-Token`
- 管理员：`X-Admin-Id` + `X-Admin-Token`
- 设备认证：Ed25519 签名（见 RFC-005）

## 数据与兼容性影响

- 未来 API v2 需保持向后兼容或提供迁移期
- 所有响应包含 `X-API-Version` 头

## 安全影响

- 必须通过 HTTPS 传输敏感数据
- 认证 token 需要定期轮换
- 实施速率限制防止暴力攻击

## 测试计划

1. 使用 curl 测试所有端点
2. 验证错误响应格式
3. 测试并发请求处理
4. 验证 Cloudflare Tunnel 穿透

## 迁移计划

不适用（新功能）

## 替代方案

1. **WebSocket**：过于复杂，不需要实时双向通信
2. **gRPC**：增加依赖复杂度，对 C++ 客户端不够友好
3. **自定义二进制协议**：调试困难，违反简单性原则

## 未解决问题

无

## 决策

使用 HTTP/HTTPS 作为 TurtleClass 的唯一网络协议，采用 URL 路径版本化（/api/v1/）。
