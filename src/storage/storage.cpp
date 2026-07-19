#include "storage.hpp"
#include <sqlite3.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <memory>

namespace turtleclass::storage {

// SQLite 存储实现
class SqliteStorage : public IStorage {
private:
    sqlite3* db_;

    bool execute_sql(const std::string& sql, const std::vector<std::string>& params = {}) {
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }

        for (size_t i = 0; i < params.size(); ++i) {
            sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_TRANSIENT);
        }

        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return rc == SQLITE_DONE || rc == SQLITE_ROW;
    }

    std::vector<std::vector<std::string>> query_sql(const std::string& sql, const std::vector<std::string>& params = {}) {
        std::vector<std::vector<std::string>> results;
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return results;
        }

        for (size_t i = 0; i < params.size(); ++i) {
            sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_TRANSIENT);
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::vector<std::string> row;
            int cols = sqlite3_column_count(stmt);
            for (int i = 0; i < cols; ++i) {
                const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                row.push_back(text ? text : "");
            }
            results.push_back(std::move(row));
        }

        sqlite3_finalize(stmt);
        return results;
    }

public:
    explicit SqliteStorage(const std::string& db_path) {
        if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) {
            throw std::runtime_error("Failed to open SQLite database: " + std::string(sqlite3_errmsg(db_)));
        }

        // 创建表结构
        init_schema();
    }

    ~SqliteStorage() override {
        sqlite3_close(db_);
    }

    void init_schema() {
        // 事件表
        execute_sql(R"(
            CREATE TABLE IF NOT EXISTS events (
                event_id TEXT PRIMARY KEY,
                device_id TEXT NOT NULL,
                device_seq INTEGER NOT NULL,
                global_seq INTEGER UNIQUE,
                timestamp INTEGER NOT NULL,
                event_type TEXT NOT NULL,
                student_id TEXT NOT NULL,
                payload TEXT NOT NULL,
                signature TEXT NOT NULL,
                is_processed INTEGER DEFAULT 0
            )
        )");

        // 设备凭证表
        execute_sql(R"(
            CREATE TABLE IF NOT EXISTS device_credentials (
                device_id TEXT PRIMARY KEY,
                class_code TEXT NOT NULL,
                public_key TEXT NOT NULL,
                encrypted_private_key TEXT,
                registered_at INTEGER NOT NULL,
                is_revoked INTEGER DEFAULT 0,
                revocation_reason TEXT
            )
        )");

        // 快照表
        execute_sql(R"(
            CREATE TABLE IF NOT EXISTS snapshots (
                snapshot_seq INTEGER PRIMARY KEY,
                created_at INTEGER NOT NULL,
                checksum TEXT NOT NULL,
                data TEXT NOT NULL,
                is_valid INTEGER DEFAULT 1
            )
        )");

        // 创建索引
        execute_sql("CREATE INDEX IF NOT EXISTS idx_events_global_seq ON events(global_seq)");
        execute_sql("CREATE INDEX IF NOT EXISTS idx_events_device_seq ON events(device_id, device_seq)");
        execute_sql("CREATE INDEX IF NOT EXISTS idx_snapshots_created ON snapshots(created_at DESC)");
    }

    // 事件操作
    bool append_event(const EventRecord& event) override {
        std::ostringstream sql;
        sql << "INSERT OR REPLACE INTO events "
            << "(event_id, device_id, device_seq, global_seq, timestamp, event_type, student_id, payload, signature, is_processed) "
            << "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

        return execute_sql(sql.str(), {
            event.event_id,
            event.device_id,
            std::to_string(event.device_seq),
            std::to_string(event.global_seq),
            std::to_string(event.timestamp),
            event.event_type,
            event.student_id,
            event.payload,
            event.signature,
            event.is_processed ? "1" : "0"
        });
    }

    std::optional<EventRecord> get_event(const std::string& event_id) override {
        auto results = query_sql(
            "SELECT event_id, device_id, device_seq, COALESCE(global_seq, 0), timestamp, event_type, student_id, payload, signature, is_processed "
            "FROM events WHERE event_id = ?",
            {event_id}
        );

        if (results.empty()) {
            return std::nullopt;
        }

        const auto& row = results[0];
        EventRecord event;
        event.event_id = row[0];
        event.device_id = row[1];
        event.device_seq = std::stoull(row[2]);
        event.global_seq = std::stoull(row[3]);
        event.timestamp = std::stoll(row[4]);
        event.event_type = row[5];
        event.student_id = row[6];
        event.payload = row[7];
        event.signature = row[8];
        event.is_processed = row[9] == "1";

        return event;
    }

    std::vector<EventRecord> get_events_since(uint64_t global_seq, size_t limit) override {
        auto results = query_sql(
            "SELECT event_id, device_id, device_seq, COALESCE(global_seq, 0), timestamp, event_type, student_id, payload, signature, is_processed "
            "FROM events WHERE global_seq > ? ORDER BY global_seq ASC LIMIT ?",
            {std::to_string(global_seq), std::to_string(limit)}
        );

        std::vector<EventRecord> events;
        for (const auto& row : results) {
            EventRecord event;
            event.event_id = row[0];
            event.device_id = row[1];
            event.device_seq = std::stoull(row[2]);
            event.global_seq = std::stoull(row[3]);
            event.timestamp = std::stoll(row[4]);
            event.event_type = row[5];
            event.student_id = row[6];
            event.payload = row[7];
            event.signature = row[8];
            event.is_processed = row[9] == "1";
            events.push_back(event);
        }

        return events;
    }

    std::optional<uint64_t> get_latest_global_seq() override {
        auto results = query_sql("SELECT MAX(global_seq) FROM events");
        if (results.empty() || results[0][0].empty()) {
            return std::nullopt;
        }
        return std::stoull(results[0][0]);
    }

    // 设备凭证操作
    bool save_device_credential(const DeviceCredential& cred) override {
        std::ostringstream sql;
        sql << "INSERT OR REPLACE INTO device_credentials "
            << "(device_id, class_code, public_key, encrypted_private_key, registered_at, is_revoked, revocation_reason) "
            << "VALUES (?, ?, ?, ?, ?, ?, ?)";

        return execute_sql(sql.str(), {
            cred.device_id,
            cred.class_code,
            cred.public_key,
            cred.encrypted_private_key,
            std::to_string(cred.registered_at),
            cred.is_revoked ? "1" : "0",
            cred.revocation_reason
        });
    }

    std::optional<DeviceCredential> get_device_credential(const std::string& device_id) override {
        auto results = query_sql(
            "SELECT device_id, class_code, public_key, COALESCE(encrypted_private_key, ''), registered_at, is_revoked, COALESCE(revocation_reason, '') "
            "FROM device_credentials WHERE device_id = ?",
            {device_id}
        );

        if (results.empty()) {
            return std::nullopt;
        }

        const auto& row = results[0];
        DeviceCredential cred;
        cred.device_id = row[0];
        cred.class_code = row[1];
        cred.public_key = row[2];
        cred.encrypted_private_key = row[3];
        cred.registered_at = std::stoll(row[4]);
        cred.is_revoked = row[5] == "1";
        cred.revocation_reason = row[6];

        return cred;
    }

    std::vector<DeviceCredential> get_all_device_credentials() override {
        auto results = query_sql(
            "SELECT device_id, class_code, public_key, COALESCE(encrypted_private_key, ''), registered_at, is_revoked, COALESCE(revocation_reason, '') "
            "FROM device_credentials"
        );

        std::vector<DeviceCredential> creds;
        for (const auto& row : results) {
            DeviceCredential cred;
            cred.device_id = row[0];
            cred.class_code = row[1];
            cred.public_key = row[2];
            cred.encrypted_private_key = row[3];
            cred.registered_at = std::stoll(row[4]);
            cred.is_revoked = row[5] == "1";
            cred.revocation_reason = row[6];
            creds.push_back(cred);
        }

        return creds;
    }

    bool revoke_device(const std::string& device_id, const std::string& reason) override {
        return execute_sql(
            "UPDATE device_credentials SET is_revoked = 1, revocation_reason = ? WHERE device_id = ?",
            {reason, device_id}
        );
    }

    // 快照操作
    bool save_snapshot(const Snapshot& snapshot) override {
        std::ostringstream sql;
        sql << "INSERT OR REPLACE INTO snapshots "
            << "(snapshot_seq, created_at, checksum, data, is_valid) "
            << "VALUES (?, ?, ?, ?, ?)";

        return execute_sql(sql.str(), {
            std::to_string(snapshot.snapshot_seq),
            std::to_string(snapshot.created_at),
            snapshot.checksum,
            snapshot.data,
            snapshot.is_valid ? "1" : "0"
        });
    }

    std::optional<Snapshot> get_latest_snapshot() override {
        auto results = query_sql(
            "SELECT snapshot_seq, created_at, checksum, data, is_valid "
            "FROM snapshots ORDER BY created_at DESC LIMIT 1"
        );

        if (results.empty()) {
            return std::nullopt;
        }

        const auto& row = results[0];
        Snapshot snapshot;
        snapshot.snapshot_seq = std::stoull(row[0]);
        snapshot.created_at = std::stoll(row[1]);
        snapshot.checksum = row[2];
        snapshot.data = row[3];
        snapshot.is_valid = row[4] == "1";

        return snapshot;
    }

    std::vector<Snapshot> get_snapshots(size_t limit) override {
        auto results = query_sql(
            "SELECT snapshot_seq, created_at, checksum, data, is_valid "
            "FROM snapshots ORDER BY created_at DESC LIMIT ?",
            {std::to_string(limit)}
        );

        std::vector<Snapshot> snapshots;
        for (const auto& row : results) {
            Snapshot snapshot;
            snapshot.snapshot_seq = std::stoull(row[0]);
            snapshot.created_at = std::stoll(row[1]);
            snapshot.checksum = row[2];
            snapshot.data = row[3];
            snapshot.is_valid = row[4] == "1";
            snapshots.push_back(snapshot);
        }

        return snapshots;
    }

    bool delete_snapshot(uint64_t snapshot_seq) override {
        return execute_sql(
            "DELETE FROM snapshots WHERE snapshot_seq = ?",
            {std::to_string(snapshot_seq)}
        );
    }

    // 维护操作
    bool compact_events() override {
        // 删除已处理的旧事件，保留最近的 N 条
        return execute_sql(R"(
            DELETE FROM events 
            WHERE is_processed = 1 
            AND global_seq < (SELECT MAX(global_seq) - 10000 FROM events)
        )");
    }

    bool export_events(const std::string& output_path) override {
        std::ofstream out(output_path);
        if (!out.is_open()) {
            return false;
        }

        auto results = query_sql(
            "SELECT event_id, device_id, device_seq, COALESCE(global_seq, 0), timestamp, event_type, student_id, payload, signature, is_processed "
            "FROM events ORDER BY global_seq ASC"
        );

        for (const auto& row : results) {
            out << row[0] << "\t" << row[1] << "\t" << row[2] << "\t" << row[3] << "\t"
                << row[4] << "\t" << row[5] << "\t" << row[6] << "\t" << row[7] << "\t"
                << row[8] << "\t" << row[9] << "\n";
        }

        return true;
    }

    bool import_events(const std::string& input_path) override {
        std::ifstream in(input_path);
        if (!in.is_open()) {
            return false;
        }

        std::string line;
        while (std::getline(in, line)) {
            std::istringstream iss(line);
            std::vector<std::string> fields;
            std::string field;

            while (std::getline(iss, field, '\t')) {
                fields.push_back(field);
            }

            if (fields.size() >= 9) {
                EventRecord event;
                event.event_id = fields[0];
                event.device_id = fields[1];
                event.device_seq = std::stoull(fields[2]);
                event.global_seq = fields[3].empty() ? 0 : std::stoull(fields[3]);
                event.timestamp = std::stoll(fields[4]);
                event.event_type = fields[5];
                event.student_id = fields[6];
                event.payload = fields[7];
                event.signature = fields[8];
                event.is_processed = fields.size() > 9 && fields[9] == "1";

                append_event(event);
            }
        }

        return true;
    }
};

// 工厂函数实现
std::unique_ptr<IStorage> create_sqlite_storage(const std::string& db_path) {
    return std::make_unique<SqliteStorage>(db_path);
}

} // namespace turtleclass::storage
