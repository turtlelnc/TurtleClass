# RFC-005：API 版本管理与冻结策略

## 状态

Accepted

## 背景

Phase 2 完成后需要冻结 API v1 和同步协议 v1。需要定义版本管理方式和冻结策略。

## 问题

如何管理 API 版本？何时冻结？如何处理 breaking changes？

## 目标

- 清晰的版本标识
- 向后兼容保证
- 平滑的升级路径
- 便于客户端适配

## 非目标

- 频繁的版本变更
- 同时支持过多旧版本
- 复杂的版本协商机制

## 提案

**决定：使用 URL 路径版本化（/api/v1/），Phase 2 结束后冻结 v1。**

### 版本标识

**URL 路径：**
```
GET /api/v1/events/download?after=12345
POST /api/v1/events/upload
```

**响应头：**
```
X-API-Version: 1.0.0
X-Protocol-Version: 1.0.0
```

### 版本号格式

采用语义化版本 `MAJOR.MINOR.PATCH`：

- **MAJOR**：不兼容的 API 变更
- **MINOR**：向后兼容的功能新增
- **PATCH**：向后兼容的问题修复

### 冻结策略

**Phase 2 结束时：**

1. 完成所有核心功能实现
2. 通过双设备同步测试
3. 通过冲突处理测试
4. 通过恢复测试
5. 冻结为 `v1.0.0`

**冻结后：**

- 只允许 PATCH 级别修复
- 不允许 breaking changes
- 新功能通过 MINOR 版本添加，保持向后兼容

### Breaking Change 处理

如需 major version 变更：

1. 创建 `/api/v2/` 端点
2. 保留 `/api/v1/` 至少 6 个月
3. 通知所有客户端升级
4. 提供迁移指南

### 协议版本

同步协议独立于 API 版本：

```json
{
  "api_version": "1.0.0",
  "protocol_version": "1.0.0",
  "client_version": "0.3.0"
}
```

### 客户端兼容性

客户端应在握手时声明支持的版本：

```json
POST /api/v1/auth/login
{
  "class_id": "...",
  "token": "...",
  "client_info": {
    "version": "0.3.0",
    "supported_api_versions": ["1.0.0"],
    "supported_protocol_versions": ["1.0.0"]
  }
}
```

服务器响应中确认使用的版本：

```json
{
  "status": "ok",
  "api_version": "1.0.0",
  "protocol_version": "1.0.0"
}
```

### 弃用策略

弃用某个端点或字段：

1. 在文档中标记为 `@deprecated`
2. 在响应中添加 `Deprecation` 头
3. 提供至少 3 个月过渡期
4. 移除前发布通知

示例：
```
Deprecation: true
Sunset: Sat, 01 Feb 2025 00:00:00 GMT
Link: </api/v2/new-endpoint>; rel="successor-version"
```

## 数据与兼容性影响

- 现有客户端需要适配新版本号
- 日志中记录 API 版本用于审计

## 安全影响

- 旧版本可能存在已知漏洞
- 应强制升级到安全版本

## 测试计划

1. 验证多版本共存
2. 验证弃用通知
3. 验证客户端版本协商
4. 验证版本不匹配错误处理

## 迁移计划

从当前无版本迁移：

1. 添加 `/api/v1/` 前缀到所有端点
2. 添加版本响应头
3. 更新文档
4. 宣布冻结

## 替代方案

1. **查询参数版本化**（`?version=1`）：不够清晰，易遗漏
2. **HTTP 头版本化**（`Accept: application/vnd.turtleclass.v1+json`）：调试困难
3. **子域名版本化**（`v1.api.turtleclass.org`）：增加 DNS 复杂度

## 未解决问题

- 是否需要自动版本协商（客户端声明支持范围，服务器选择最佳）

## 决策

使用 URL 路径版本化（`/api/v1/`），Phase 2 结束后冻结为 `v1.0.0`，breaking changes 通过新 major version 处理，保留旧版本至少 6 个月。
