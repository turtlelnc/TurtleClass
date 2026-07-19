#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <sqlite3.h>

namespace turtleclass {

/**
 * 离线事件记录
 */
struct OfflineEvent {
    int64_t id;
    std::string event_json;
    std::string signature;
    int64_t sequence_number;
    std::string device_id;
    int64_t timestamp;
    bool synced;
};

/**
 * 本地 SQLite 存储 - 用于 Windows 客户端离线数据持久化
 * 
 * 功能：
 * - 离线事件队列持久化
 * - 登录凭据安全存储
 * - 同步状态缓存
 * - 冲突暂存区
 */
class TurtleClassStorage {
public:
    static TurtleClassStorage& instance();

    // 初始化数据库
    bool initialize(const std::string& db_path);

    // 关闭数据库
    void close();

    // 离线事件队列操作
    bool add_offline_event(const std::string& event_json, 
                           const std::string& signature,
                           int64_t sequence_number,
                           const std::string& device_id);
    
    std::vector<OfflineEvent> get_pending_events(int limit = 100);
    
    bool mark_event_synced(int64_t event_id);
    
    bool remove_event(int64_t event_id);
    
    int get_pending_event_count();

    // 凭据存储
    bool save_credentials(const std::string& class_code,
                         const std::string& student_id,
                         const std::string& encrypted_password,
                         const std::string& device_token);
    
    bool load_credentials(std::string& class_code,
                         std::string& student_id,
                         std::string& encrypted_password,
                         std::string& device_token);
    
    bool clear_credentials();

    // 同步状态缓存
    bool save_sync_state(int64_t last_uploaded_seq,
                        int64_t last_downloaded_seq,
                        int64_t server_snapshot_seq);
    
    bool load_sync_state(int64_t& last_uploaded_seq,
                        int64_t& last_downloaded_seq,
                        int64_t& server_snapshot_seq);

    // 冲突暂存
    bool add_conflict(const std::string& conflict_json);
    
    std::vector<std::string> get_conflicts();
    
    bool clear_conflicts();

    // 工具函数
    bool execute_sql(const std::string& sql);
    
    sqlite3* get_db() { return db_; }

private:
    TurtleClassStorage();
    ~TurtleClassStorage();

    bool create_tables();

    mutable std::mutex mutex_;
    sqlite3* db_;
    bool initialized_;
};

} // namespace turtleclass
