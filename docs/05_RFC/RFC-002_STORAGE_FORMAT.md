# RFC-002：TSV 追加式日志 + 快照文件格式

## 状态

Accepted

## 背景

Phase 2 需要持久化事件日志和快照系统。当前使用内存存储，重启后数据丢失。需要定义正式的存储格式。

## 问题

选择何种存储格式用于事件日志和快照？

## 目标

- 简单、可读、易调试
- 支持追加写入（高性能）
- 支持增量备份
- 支持损坏检测
- 便于手动修复

## 非目标

- 复杂的事务支持
- 随机读写优化
- 压缩存储（可后期添加）

## 提案

**决定：使用 TSV（Tab-Separated Values）追加式日志 + JSON 快照格式。**

### 事件日志格式（events.tsv）

每行一个事件记录，字段以制表符分隔：

```
server_sequence<TAB>server_time_unix<TAB>event_id<TAB>class_id<TAB>event_group_id<TAB>target_id<TAB>device_id<TAB>device_local_sequence<TAB>event_type<TAB>points_delta<TAB>badge_delta<TAB>rule_version<TAB>previous_hash<TAB>compensates_event_id<TAB>signature
```

字段说明：

| 序号 | 字段 | 类型 | 说明 |
|---|---|---|---|
| 1 | server_sequence | int64 | 服务端全局序号 |
| 2 | server_time_unix | int64 | 服务端确认时间（Unix 秒） |
| 3 | event_id | hex | 事件 ID（hex 编码） |
| 4 | class_id | hex | 班级 ID（hex 编码） |
| 5 | event_group_id | hex | 事件组 ID（hex 编码） |
| 6 | target_id | hex | 学生 ID（hex 编码） |
| 7 | device_id | hex | 设备 ID（hex 编码） |
| 8 | device_local_sequence | int64 | 设备本地序号 |
| 9 | event_type | int | 事件类型枚举值 |
| 10 | points_delta | int | 积分变化 |
| 11 | badge_delta | int | 徽章变化 |
| 12 | rule_version | int32 | 规则版本 |
| 13 | previous_hash | hex | 前一事件哈希（链校验） |
| 14 | compensates_event_id | hex | 被补偿的事件 ID（可选） |
| 15 | signature | hex | Ed25519 签名（见 RFC-005） |

### 快照文件格式（snapshot.json）

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

### 目录结构

```
data/
├── events.tsv           # 主事件日志
├── snapshot.json        # 最新快照
├── backups/
│   ├── events-<unix>.tsv
│   └── snapshot-<unix>.json
└── exports/
    └── ...
```

### 损坏检测

1. **行校验**：每行必须有 15 个字段
2. **序号连续性**：server_sequence 必须连续递增
3. **哈希链校验**：previous_hash 必须匹配前一事件的 blake3 哈希
4. **签名校验**：每个事件必须有有效的 Ed25519 签名
5. **快照校验**：JSON 文件包含 checksum 字段

### 恢复策略

1. 优先从 snapshot.json 加载基线状态
2. 重放 snapshot.last_server_sequence 之后的事件
3. 如果快照损坏，从头重放所有事件
4. 如果事件日志损坏，尝试从备份恢复

## 数据与兼容性影响

- 未来版本增加字段时，追加到末尾，保持向后兼容
- 读取器忽略未知字段
- 版本号记录在快照文件中

## 安全影响

- 签名防止篡改
- 哈希链检测插入/删除
- 备份文件应加密存储（后期实现）

## 测试计划

1. 写入大量事件验证追加性能
2. 模拟文件损坏验证恢复逻辑
3. 验证哈希链校验
4. 验证签名验证

## 迁移计划

从当前内存存储迁移：

1. 启动时创建空 events.tsv
2. 将内存事件按顺序写入
3. 生成初始 snapshot.json

## 替代方案

1. **SQLite**：过于复杂，不需要事务支持
2. **二进制格式**：调试困难，不易手动修复
3. **纯 JSON 日志**：文件体积大，解析慢

## 未解决问题

无

## 决策

使用 TSV 追加式日志存储事件，JSON 格式存储快照，blake3 用于完整性校验，Ed25519 用于签名。
