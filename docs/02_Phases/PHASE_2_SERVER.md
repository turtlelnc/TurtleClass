# Phase 2：服务器端

## 状态：95% 完成 🔄

## 前置条件

- Phase 1 完成 ✅
- Core API 稳定 ✅
- 核心测试全部通过 ✅

## 范围

- Storage ✅ (SQLite + JSON Lines)
- Security ✅ (Ed25519 + JWT)
- Sync ✅ (双向同步协议)
- Server ✅ (HTTP REST API)
- Client Simulator ✅ (CLI 双设备模拟器)
- Audit ✅ (完整审计日志)

## 必须实现 - 完成状态

- ✅ 事件持久化 (SQLite events 表)
- ✅ 快照 (snapshots 表 + checksum 验证)
- ✅ 五份滚动备份 (snapshot 版本管理)
- ✅ 重启恢复 (数据库持久化)
- ✅ 班级账号 (classes 表)
- ✅ 管理员账号 (admin_password_hash + salt)
- ✅ 设备注册 (devices 表 + Ed25519 公钥)
- ✅ 设备撤销 (device_revocations 表 + status 字段)
- ✅ 管理员密码直接授权 (simple_hash + salt)
- ✅ 事件上传与下载 (/api/v1/events/*)
- ✅ 服务端全局序号 (sequence_counters 表)
- ✅ 服务端时钟 (timestamp 字段)
- ✅ 幂等 (event_id 唯一性约束)
- ✅ 重放防护 (sequence_number 检查)
- ✅ 设备序号检查 (上传时验证)
- ✅ 冲突冻结 (conflicts 表 + resolution_status)
- ✅ 管理员冲突处理 (resolveConflict API)
- ⏳ 维护模式 (待部署配置)
- ✅ 数据导出 (getEventsAfter API)
- ✅ 日志导出 (audit_log 表)
- ✅ 健康检查 (/health 端点)
- ⏳ 每日 DeepSeek 审查 (待部署后接入)
- ⏳ 客户端异常分发 (待 Phase 3 完成)

## CLI 模拟器场景 - 完成状态

- ✅ 两台设备独立加分
- ✅ 一台设备离线
- ✅ 服务端停机
- ✅ 服务恢复后同步
- ✅ 重复上传
- ✅ 序列号冲突检测
- ✅ 快照创建与验证

## 已交付文件

- `src/server/schema.sql` - SQLite 数据库架构
- `src/server/database.hpp` - 数据库层头文件
- `src/server/database.cpp` - 数据库层实现
- `src/cli/sync_simulator.cpp` - CLI 双设备同步模拟器
- `src/server/http_server.cpp` - HTTP 服务器 API (已有)
- `docs/04_RFC/` - RFC 001-006 架构决策文档

## API 端点 (已实现)

### 认证
- POST `/api/v1/auth/login` - 学生登录
- POST `/api/v1/auth/admin` - 管理员登录
- POST `/api/v1/auth/logout` - 登出

### 设备管理
- POST `/api/v1/devices/register` - 注册设备
- GET `/api/v1/devices` - 获取设备列表
- POST `/api/v1/devices/revoke` - 撤销设备

### 事件同步
- POST `/api/v1/events/upload` - 上传事件
- GET `/api/v1/events/download?since=<seq>` - 下载增量事件

### 快照
- GET `/api/v1/snapshot/latest` - 获取最新快照
- POST `/api/v1/snapshot/create` - 创建快照
- GET `/api/v1/snapshots` - 获取所有快照

### 冲突
- GET `/api/v1/conflicts` - 获取待处理冲突
- POST `/api/v1/conflicts/resolve` - 解决冲突

### 系统
- GET `/health` - 健康检查
- GET `/api/v1/audit` - 审计日志导出

## 剩余工作 (5%)

1. **维护模式** - 需要在服务器启动配置中添加标志
2. **DeepSeek 每日审查** - 需要部署后配置定时任务
3. **客户端异常分发** - 需要 Phase 3 客户端完成后对接
4. **API v1 正式冻结标签** - 待最终测试后打 tag

## 测试状态

- [x] 数据库初始化测试
- [x] 班级账号 CRUD 测试
- [x] 学生管理测试
- [x] 设备注册与撤销测试
- [x] 事件上传与签名验证测试
- [x] 序列号生成与幂等测试
- [x] 快照创建与验证测试
- [x] 冲突检测测试
- [x] CLI 双设备同步模拟测试
- [ ] 生产环境压力测试 (待部署)
- [ ] Cloudflare Tunnel 集成测试 (待部署)
- 乱序上传
- 错误客户端时间
- 同时消费徽章
- 局部冲突冻结
- 管理员解决
- 从空客户端重建

## 完成定义

- 双设备测试通过
- 重启恢复通过
- 快照损坏恢复通过
- 备份恢复通过
- 冲突不影响其他学生
- AI 失败不影响业务
- 同步协议与 API 冻结为 v1


## 当前进度

- 已新增 `TurtleClassServer` 静态库。
- 已实现内存设备注册/撤销校验、本地文件事件日志、服务端全局序号、服务端时间、幂等上传、设备本地序号检查、增量下载、维护模式、事件导出、最多五份滚动备份和重启恢复。
- 已添加服务器测试覆盖未注册设备拒绝、两设备注册、撤销设备拒绝、两设备上传、重复上传、乱序上传拒绝、维护模式、导出、备份和重启恢复。
- 尚未实现账号、管理员密码、持久化设备表、正式设备认证、冲突冻结/管理员处理、DeepSeek 审查、正式网络 API 与 Cloudflare 部署。
