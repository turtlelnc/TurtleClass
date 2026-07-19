#include "Storage.hpp"
#include <iostream>
#include <sstream>

namespace turtleclass {

TurtleClassStorage& TurtleClassStorage::instance() {
    static TurtleClassStorage instance;
    return instance;
}

TurtleClassStorage::TurtleClassStorage() : db_(nullptr), initialized_(false) {
}

TurtleClassStorage::~TurtleClassStorage() {
    close();
}

bool TurtleClassStorage::initialize(const std::string& db_path) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        std::cerr << "[Storage] Already initialized" << std::endl;
        return false;
    }

    int rc = sqlite3_open(db_path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "[Storage] Cannot open database: " << sqlite3_errmsg(db_) << std::endl;
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }

    if (!create_tables()) {
        std::cerr << "[Storage] Failed to create tables" << std::endl;
        close();
        return false;
    }

    initialized_ = true;
    std::cout << "[Storage] Initialized successfully: " << db_path << std::endl;
    return true;
}

void TurtleClassStorage::close() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
        initialized_ = false;
        std::cout << "[Storage] Database closed" << std::endl;
    }
}

bool TurtleClassStorage::create_tables() {
    const char* sql = R"(
        -- 离线事件队列
        CREATE TABLE IF NOT EXISTS offline_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            event_json TEXT NOT NULL,
            signature TEXT NOT NULL,
            sequence_number INTEGER NOT NULL,
            device_id TEXT NOT NULL,
            timestamp INTEGER NOT NULL,
            synced INTEGER DEFAULT 0,
            created_at INTEGER DEFAULT (strftime('%s', 'now'))
        );

        CREATE INDEX IF NOT EXISTS idx_offline_events_synced ON offline_events(synced);
        CREATE INDEX IF NOT EXISTS idx_offline_events_sequence ON offline_events(sequence_number);

        -- 凭据存储
        CREATE TABLE IF NOT EXISTS credentials (
            id INTEGER PRIMARY KEY,
            class_code TEXT NOT NULL,
            student_id TEXT NOT NULL,
            encrypted_password TEXT,
            device_token TEXT,
            updated_at INTEGER DEFAULT (strftime('%s', 'now'))
        );

        -- 同步状态缓存
        CREATE TABLE IF NOT EXISTS sync_state (
            id INTEGER PRIMARY KEY,
            last_uploaded_seq INTEGER DEFAULT 0,
            last_downloaded_seq INTEGER DEFAULT 0,
            server_snapshot_seq INTEGER DEFAULT 0,
            updated_at INTEGER DEFAULT (strftime('%s', 'now'))
        );

        -- 冲突暂存区
        CREATE TABLE IF NOT EXISTS conflicts (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            conflict_json TEXT NOT NULL,
            created_at INTEGER DEFAULT (strftime('%s', 'now'))
        );

        -- 初始化默认值
        INSERT OR IGNORE INTO sync_state (id, last_uploaded_seq, last_downloaded_seq, server_snapshot_seq) 
        VALUES (1, 0, 0, 0);
    )";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[Storage] SQL error creating tables: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

bool TurtleClassStorage::add_offline_event(const std::string& event_json,
                                           const std::string& signature,
                                           int64_t sequence_number,
                                           const std::string& device_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !db_) {
        return false;
    }

    int64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    const char* sql = R"(
        INSERT INTO offline_events (event_json, signature, sequence_number, device_id, timestamp, synced)
        VALUES (?, ?, ?, ?, ?, 0)
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Storage] Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, event_json.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, signature.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, sequence_number);
    sqlite3_bind_text(stmt, 4, device_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 5, timestamp);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "[Storage] Failed to insert event: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    return true;
}

std::vector<OfflineEvent> TurtleClassStorage::get_pending_events(int limit) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<OfflineEvent> events;

    if (!initialized_ || !db_) {
        return events;
    }

    std::stringstream ss;
    ss << "SELECT id, event_json, signature, sequence_number, device_id, timestamp, synced "
       << "FROM offline_events WHERE synced = 0 ORDER BY sequence_number LIMIT ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, ss.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Storage] Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return events;
    }

    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        OfflineEvent event;
        event.id = sqlite3_column_int64(stmt, 0);
        event.event_json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        event.signature = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        event.sequence_number = sqlite3_column_int64(stmt, 3);
        event.device_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        event.timestamp = sqlite3_column_int64(stmt, 5);
        event.synced = sqlite3_column_int(stmt, 6) != 0;

        events.push_back(event);
    }

    sqlite3_finalize(stmt);
    return events;
}

bool TurtleClassStorage::mark_event_synced(int64_t event_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !db_) {
        return false;
    }

    const char* sql = "UPDATE offline_events SET synced = 1 WHERE id = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Storage] Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    sqlite3_bind_int64(stmt, 1, event_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool TurtleClassStorage::remove_event(int64_t event_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !db_) {
        return false;
    }

    const char* sql = "DELETE FROM offline_events WHERE id = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Storage] Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    sqlite3_bind_int64(stmt, 1, event_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

int TurtleClassStorage::get_pending_event_count() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !db_) {
        return 0;
    }

    const char* sql = "SELECT COUNT(*) FROM offline_events WHERE synced = 0";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Storage] Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return 0;
    }

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count;
}

bool TurtleClassStorage::save_credentials(const std::string& class_code,
                                          const std::string& student_id,
                                          const std::string& encrypted_password,
                                          const std::string& device_token) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !db_) {
        return false;
    }

    const char* sql = R"(
        INSERT OR REPLACE INTO credentials (id, class_code, student_id, encrypted_password, device_token, updated_at)
        VALUES (1, ?, ?, ?, ?, strftime('%s', 'now'))
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Storage] Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, class_code.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, student_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, encrypted_password.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, device_token.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool TurtleClassStorage::load_credentials(std::string& class_code,
                                          std::string& student_id,
                                          std::string& encrypted_password,
                                          std::string& device_token) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !db_) {
        return false;
    }

    const char* sql = "SELECT class_code, student_id, encrypted_password, device_token FROM credentials WHERE id = 1";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Storage] Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        class_code = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        student_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        encrypted_password = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        device_token = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

bool TurtleClassStorage::clear_credentials() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !db_) {
        return false;
    }

    const char* sql = "DELETE FROM credentials WHERE id = 1";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Storage] Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool TurtleClassStorage::save_sync_state(int64_t last_uploaded_seq,
                                         int64_t last_downloaded_seq,
                                         int64_t server_snapshot_seq) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !db_) {
        return false;
    }

    const char* sql = R"(
        UPDATE sync_state 
        SET last_uploaded_seq = ?, 
            last_downloaded_seq = ?, 
            server_snapshot_seq = ?,
            updated_at = strftime('%s', 'now')
        WHERE id = 1
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Storage] Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    sqlite3_bind_int64(stmt, 1, last_uploaded_seq);
    sqlite3_bind_int64(stmt, 2, last_downloaded_seq);
    sqlite3_bind_int64(stmt, 3, server_snapshot_seq);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool TurtleClassStorage::load_sync_state(int64_t& last_uploaded_seq,
                                         int64_t& last_downloaded_seq,
                                         int64_t& server_snapshot_seq) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !db_) {
        return false;
    }

    const char* sql = "SELECT last_uploaded_seq, last_downloaded_seq, server_snapshot_seq FROM sync_state WHERE id = 1";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Storage] Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        last_uploaded_seq = sqlite3_column_int64(stmt, 0);
        last_downloaded_seq = sqlite3_column_int64(stmt, 1);
        server_snapshot_seq = sqlite3_column_int64(stmt, 2);
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

bool TurtleClassStorage::add_conflict(const std::string& conflict_json) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !db_) {
        return false;
    }

    const char* sql = "INSERT INTO conflicts (conflict_json) VALUES (?)";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Storage] Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, conflict_json.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

std::vector<std::string> TurtleClassStorage::get_conflicts() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> conflicts;

    if (!initialized_ || !db_) {
        return conflicts;
    }

    const char* sql = "SELECT conflict_json FROM conflicts ORDER BY created_at DESC";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Storage] Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return conflicts;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        conflicts.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }

    sqlite3_finalize(stmt);
    return conflicts;
}

bool TurtleClassStorage::clear_conflicts() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !db_) {
        return false;
    }

    const char* sql = "DELETE FROM conflicts";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Storage] Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool TurtleClassStorage::execute_sql(const std::string& sql) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !db_) {
        return false;
    }

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[Storage] SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

} // namespace turtleclass
