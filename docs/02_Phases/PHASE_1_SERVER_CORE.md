# Phase 1：服务器核心

## 目标

建立可在 macOS 编译的跨平台 C++ 核心，为后续服务端和 WinUI 提供唯一业务实现。

## 本阶段目标

- C++23
- CMake
- Core
- 内存事件仓库
- 领域测试
- 确定性一致性检查

## 建议目标

- `TurtleClass.Core`
- `TurtleClass.Tests`

可选：

- `TurtleClass.Core.Examples`

## 必须实现的类型

- StrongId
- StudentId
- DeviceId
- EventId
- EventGroupId
- RuleSet
- LevelRule
- StudentState
- DomainEvent
- EventGroup
- InMemoryEventStore
- StateProjector
- DomainService
- ConsistencyChecker

## 必须实现的规则

- 默认八级来自配置
- 多级升级
- 多级降级
- 最低等级负分
- 升级发徽章
- 降级收徽章
- 徽章允许负数
- 规则修改后重算
- 事件不可变
- 事件组原子性
- 补偿事件撤销
- 撤销一次撤销
- 重放得到一致状态

## 测试最低集合

- 单次升级
- 单次降级
- 多级升级
- 多级降级
- 最低等级负分
- 负分恢复
- 升级发放徽章
- 降级收回徽章
- 徽章负数
- 修改阈值后重算
- 增加等级后重算
- 删除等级后重算
- 普通撤销
- 模板整体撤销
- 撤销的撤销
- 重复事件
- 无效事件
- 重放一致性
- 事件组失败不产生部分状态

## 禁止范围

- WinUI
- 正式网络
- Cloudflare
- DeepSeek
- 正式密码系统
- 正式磁盘格式
- 平台专属 API

## 完成定义

- macOS 编译通过
- 全部测试通过
- 公开 API 有文档
- 无业务逻辑旁路
- PROJECT_MASTER 与 HANDOFF 已更新


## 当前进度

- 已建立 `TurtleClassCore` 静态库与 `TurtleClassTests` 测试目标。
- 已实现 StrongId、规则集、学生状态、领域事件、事件组、内存事件仓库、状态投影、领域服务与一致性检查。
- 已验证升级、降级、最低等级负分、徽章负数、规则重算、事件组原子性、补偿撤销、撤销的撤销、重复事件和重放一致性。
- 当前阶段仍禁止 WinUI、正式网络、Cloudflare、DeepSeek、正式密码系统和正式磁盘格式。
