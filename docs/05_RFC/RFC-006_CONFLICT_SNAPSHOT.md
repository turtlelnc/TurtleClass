# RFC-006：冲突检测与快照系统

## 状态

Accepted

## 背景

Phase 2 需要实现冲突检测、局部资产冻结和快照系统。多设备离线操作可能导致同一学生的状态在不同设备上产生分歧。

## 问题

如何检测冲突？如何冻结冲突资产？如何实现快照和恢复？

## 目标

- 自动检测同一学生的状态冲突
- 只冻结冲突学生，不影响其他学生
- 支持管理员解决冲突
- 定期创建快照用于快速恢复
- 检测快照损坏并自动修复

## 非目标

- 自动合并冲突（需要管理员决策）
- 全局冻结（影响太大）
- 复杂的冲突解决策略

## 提案

### 冲突检测

**触发条件：**

1. 服务器收到事件组时检查
2. 同一学生在不同设备上的事件导致状态不一致

**检测规则：**

```
IF (设备 A 的事件修改了学生 S) 
AND (设备 B 在设备 A 之后也修改了学生 S)
AND (两个事件的设备本地序号不连续)
THEN 标记为学生 S 的潜在冲突
```

**冲突记录格式：**

```json
{
  "conflict_id": "conflict-uuid",
  "student_id": "student-uuid",
  "detected_at_unix": 1721400000,
  "devices_involved": ["device-a-id", "device-b-id"],
  "divergent_events": [
    {
      "event_id": "...",
      "device_id": "...",
      "server_sequence": 123,
      "points_delta": 5,
      "badge_delta": 0
    },
    {
      "event_id": "...",
      "device_id": "...",
      "server_sequence": 125,
      "points_delta": -3,
      "badge_delta": 0
    }
  ],
  "status": "pending|resolved",
  "resolution": null
}
```

### 局部资产冻结

**冻结范围：**

- 只冻结冲突学生的积分、等级和徽章
- 其他学生正常操作
- 冲突学生仍可查看详情，但不能修改

**冻结标识：**

在学生状态中添加 `frozen: true` 字段。

**API 响应：**

上传涉及冻结学生的事件时，返回特殊错误：
```json
{
  "accepted": false,
  "errors": ["Student <id> is frozen due to conflict"]
}
```

### 管理员解决冲突

**解决流程：**

1. 管理员查看冲突列表
2. 选择要保留的事件版本
3. 确认解决
4. 服务器更新学生状态，解除冻结
5. 记录解决决策到审计日志

**API：**

```
POST /api/v1/conflicts/{conflict_id}/resolve
{
  "keep_event_id": "event-uuid",
  "discard_event_ids": ["event-uuid-2"],
  "admin_note": "Keep the earlier event"
}
```

### 快照系统

**快照内容：**

```json
{
  "version": 1,
  "created_at_unix": 1721400000,
  "last_server_sequence": 12345,
  "rule_set": {
    "id": "ruleset-1",
    "version": 1,
    "levels": [...]
  },
  "students": {
    "student-id-hex": {
      "level_index": 2,
      "points_in_level": 15,
      "badges": 5,
      "frozen": false
    }
  },
  "devices": [...],
  "conflicts": [...],
  "checksum": "blake3-hex"
}
```

**创建时机：**

- 每 100 个事件自动创建
- 管理员手动触发
- 服务器关闭前

**损坏检测：**

1. JSON 解析失败
2. checksum 不匹配
3. last_server_sequence 与事件日志不连续

**恢复策略：**

1. 如果最新快照损坏，尝试上一个备份
2. 如果所有快照损坏，从头重放事件日志
3. 如果事件日志也损坏，报告无法恢复

## 数据与兼容性影响

- 学生状态增加 frozen 字段
- 事件日志增加冲突标记（可选）
- 新增快照文件

## 安全影响

- 只有管理员可以解决冲突
- 冲突解决必须审计
- 快照 checksum 防止篡改

## 测试计划

1. 模拟双设备修改同一学生
2. 验证冲突检测
3. 验证冻结生效
4. 验证管理员解决流程
5. 模拟快照损坏验证恢复

## 迁移计划

新功能，无需迁移。

## 替代方案

1. **乐观合并**：可能丢失数据，不适合积分系统
2. **最后写入获胜**：不公平，可能丢失重要操作
3. **全局冻结**：影响太大

## 未解决问题

- 是否需要冲突自动通知客户端

## 决策

使用基于事件的冲突检测，局部冻结冲突学生，管理员手动解决冲突，定期创建带 checksum 的 JSON 快照。
