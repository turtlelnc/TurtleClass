#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <memory>

namespace turtleclass::storage {

// 事件记录结构（用于 JSON Lines 存储）
struct EventRecord {
    std::string event_id;      // 全局唯一事件 ID (UUID)
    std::string device_id;     // 设备 ID
    uint64_t device_seq;       // 设备本地序号
    uint64_t global_seq;       // 服务端全局序号（上传时分配）
    int64_t timestamp;         // 事件时间戳（毫秒）
    std::string event_type;    // 事件类型
    std::string student_id;    // 学生 ID
    std::string payload;       // JSON 格式的事件数据
    std::string signature;     // Ed25519 签名（hex 编码）
    bool is_processed;         // 是否已处理
};

// 快照结构
struct Snapshot {
    uint64_t snapshot_seq;           // 快照对应的全局序号
    int64_t created_at;              // 创建时间戳
    std::string checksum;            // SHA-256 校验和
    std::string data;                // 压缩后的状态数据（base64）
    bool is_valid;                   // 快照是否有效
};

// 设备凭证结构
struct DeviceCredential {
    std::string device_id;           // 设备 ID
    std::string class_code;          // 班级代码
    std::string public_key;          // Ed25519 公钥（hex 编码）
    std::string encrypted_private_key; // 加密的私钥（hex 编码）
    int64_t registered_at;           // 注册时间戳
    bool is_revoked;                 // 是否已撤销
    std::string revocation_reason;   // 撤销原因
};

// 抽象存储接口
class IStorage {
public:
    virtual ~IStorage() = default;

    // 事件操作
    virtual bool append_event(const EventRecord& event) = 0;
    virtual std::optional<EventRecord> get_event(const std::string& event_id) = 0;
    virtual std::vector<EventRecord> get_events_since(uint64_t global_seq, size_t limit = 1000) = 0;
    virtual std::optional<uint64_t> get_latest_global_seq() = 0;

    // 设备凭证操作
    virtual bool save_device_credential(const DeviceCredential& cred) = 0;
    virtual std::optional<DeviceCredential> get_device_credential(const std::string& device_id) = 0;
    virtual std::vector<DeviceCredential> get_all_device_credentials() = 0;
    virtual bool revoke_device(const std::string& device_id, const std::string& reason) = 0;

    // 快照操作
    virtual bool save_snapshot(const Snapshot& snapshot) = 0;
    virtual std::optional<Snapshot> get_latest_snapshot() = 0;
    virtual std::vector<Snapshot> get_snapshots(size_t limit = 5) = 0;
    virtual bool delete_snapshot(uint64_t snapshot_seq) = 0;

    // 维护操作
    virtual bool compact_events() = 0;
    virtual bool export_events(const std::string& output_path) = 0;
    virtual bool import_events(const std::string& input_path) = 0;
};

// 工厂函数：创建 SQLite 存储实现
std::unique_ptr<IStorage> create_sqlite_storage(const std::string& db_path);

} // namespace turtleclass::storage
