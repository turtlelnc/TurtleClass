#include "database.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

namespace turtleclass {
namespace server {

Database::Database() : db_(nullptr) {}

Database::~Database() {
    close();
}

bool Database::initialize(const std::string& db_path) {
    if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) {
        std::cerr << "Failed to open database: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    // Load and execute schema
    std::ifstream schema_file("src/server/schema.sql");
    if (!schema_file.is_open()) {
        std::cerr << "Failed to open schema.sql" << std::endl;
        close();
        return false;
    }

    std::stringstream buffer;
    buffer << schema_file.rdbuf();
    std::string schema = buffer.str();

    char* errMsg = nullptr;
    if (sqlite3_exec(db_, schema.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to execute schema: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        close();
        return false;
    }

    return true;
}

void Database::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool Database::executeSql(const std::string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool Database::tableExists(const std::string& table_name) {
    std::string sql = "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='" + table_name + "'";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = sqlite3_column_int(stmt, 0) > 0;
    }
    sqlite3_finalize(stmt);
    return exists;
}

// Class operations
bool Database::createClass(const std::string& class_code, const std::string& class_name,
                           const std::string& admin_password_hash, const std::string& admin_salt) {
    std::string sql = "INSERT INTO classes (class_code, class_name, admin_password_hash, admin_salt) "
                      "VALUES ('" + class_code + "', '" + class_name + "', '" + 
                      admin_password_hash + "', '" + admin_salt + "')";
    return executeSql(sql);
}

std::optional<ClassInfo> Database::getClassByCode(const std::string& class_code) {
    std::string sql = "SELECT id, class_code, class_name, admin_password_hash, admin_salt FROM classes WHERE class_code = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, class_code.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ClassInfo info;
        info.id = sqlite3_column_int(stmt, 0);
        info.class_code = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        info.class_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        info.admin_password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        info.admin_salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        sqlite3_finalize(stmt);
        return info;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

bool Database::updateClass(int class_id, const std::string& class_name) {
    std::string sql = "UPDATE classes SET class_name = '" + class_name + 
                      "', updated_at = CURRENT_TIMESTAMP WHERE id = " + std::to_string(class_id);
    return executeSql(sql);
}

// Student operations
bool Database::addStudent(int class_id, const std::string& student_id, const std::string& student_name,
                          const std::string& password_hash, const std::string& password_salt) {
    std::string sql = "INSERT INTO students (class_id, student_id, student_name, password_hash, password_salt) "
                      "VALUES (" + std::to_string(class_id) + ", '" + student_id + "', '" + 
                      student_name + "', '" + password_hash + "', '" + password_salt + "')";
    return executeSql(sql);
}

std::optional<StudentInfo> Database::getStudent(int class_id, const std::string& student_id) {
    std::string sql = "SELECT id, class_id, student_id, student_name, password_hash, password_salt FROM students "
                      "WHERE class_id = ? AND student_id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, class_id);
    sqlite3_bind_text(stmt, 2, student_id.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        StudentInfo info;
        info.id = sqlite3_column_int(stmt, 0);
        info.class_id = sqlite3_column_int(stmt, 1);
        info.student_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        info.student_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        info.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        info.password_salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        sqlite3_finalize(stmt);
        return info;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<StudentInfo> Database::getAllStudents(int class_id) {
    std::vector<StudentInfo> students;
    std::string sql = "SELECT id, class_id, student_id, student_name, password_hash, password_salt FROM students "
                      "WHERE class_id = ? ORDER BY student_id";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return students;
    }

    sqlite3_bind_int(stmt, 1, class_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        StudentInfo info;
        info.id = sqlite3_column_int(stmt, 0);
        info.class_id = sqlite3_column_int(stmt, 1);
        info.student_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        info.student_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        info.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        info.password_salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        students.push_back(info);
    }

    sqlite3_finalize(stmt);
    return students;
}

bool Database::updateStudent(int class_id, const std::string& student_id, const std::string& student_name) {
    std::string sql = "UPDATE students SET student_name = '" + student_name + 
                      "' WHERE class_id = " + std::to_string(class_id) + " AND student_id = '" + student_id + "'";
    return executeSql(sql);
}

bool Database::removeStudent(int class_id, const std::string& student_id) {
    std::string sql = "DELETE FROM students WHERE class_id = " + std::to_string(class_id) + 
                      " AND student_id = '" + student_id + "'";
    return executeSql(sql);
}

// Device operations
bool Database::registerDevice(const std::string& device_id, const std::string& device_name,
                              const std::string& device_type, int class_id, const std::string& public_key) {
    std::string sql = "INSERT INTO devices (device_id, device_name, device_type, class_id, public_key) "
                      "VALUES ('" + device_id + "', '" + device_name + "', '" + device_type + "', " + 
                      std::to_string(class_id) + ", '" + public_key + "')";
    return executeSql(sql);
}

std::optional<DeviceInfo> Database::getDevice(const std::string& device_id) {
    std::string sql = "SELECT id, device_id, device_name, device_type, class_id, public_key, status, last_seen FROM devices "
                      "WHERE device_id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, device_id.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        DeviceInfo info;
        info.id = sqlite3_column_int(stmt, 0);
        info.device_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        info.device_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        info.device_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        info.class_id = sqlite3_column_type(stmt, 4) == SQLITE_NULL ? 
                        std::nullopt : std::make_optional(sqlite3_column_int(stmt, 4));
        info.public_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        info.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        info.last_seen = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        sqlite3_finalize(stmt);
        return info;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<DeviceInfo> Database::getAllDevices(int class_id) {
    std::vector<DeviceInfo> devices;
    std::string sql = "SELECT id, device_id, device_name, device_type, class_id, public_key, status, last_seen FROM devices "
                      "WHERE class_id = ? ORDER BY created_at";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return devices;
    }

    sqlite3_bind_int(stmt, 1, class_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DeviceInfo info;
        info.id = sqlite3_column_int(stmt, 0);
        info.device_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        info.device_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        info.device_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        info.class_id = sqlite3_column_type(stmt, 4) == SQLITE_NULL ? 
                        std::nullopt : std::make_optional(sqlite3_column_int(stmt, 4));
        info.public_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        info.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        info.last_seen = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        devices.push_back(info);
    }

    sqlite3_finalize(stmt);
    return devices;
}

bool Database::revokeDevice(const std::string& device_id, const std::string& reason, const std::string& revoked_by) {
    std::string sql = "UPDATE devices SET status = 'revoked', revoked_at = CURRENT_TIMESTAMP "
                      "WHERE device_id = '" + device_id + "'";
    if (!executeSql(sql)) {
        return false;
    }

    sql = "INSERT INTO device_revocations (device_id, revocation_reason, revoked_by) "
          "VALUES ('" + device_id + "', '" + reason + "', '" + revoked_by + "')";
    return executeSql(sql);
}

bool Database::updateDeviceLastSeen(const std::string& device_id) {
    std::string sql = "UPDATE devices SET last_seen = CURRENT_TIMESTAMP WHERE device_id = '" + device_id + "'";
    return executeSql(sql);
}

// Event operations
bool Database::insertEvent(const std::string& event_id, int class_id, const std::string& device_id,
                           const std::string& event_type, const std::string& event_data,
                           const std::string& signature, const std::string& timestamp, int sequence_number) {
    std::string sql = "INSERT INTO events (event_id, class_id, device_id, event_type, event_data, signature, timestamp, sequence_number) "
                      "VALUES ('" + event_id + "', " + std::to_string(class_id) + ", '" + device_id + "', '" + 
                      event_type + "', '" + event_data + "', '" + signature + "', '" + timestamp + "', " + 
                      std::to_string(sequence_number) + ")";
    return executeSql(sql);
}

std::vector<EventInfo> Database::getEventsAfter(int class_id, int sequence_number) {
    std::vector<EventInfo> events;
    std::string sql = "SELECT id, event_id, class_id, device_id, event_type, event_data, signature, timestamp, sequence_number FROM events "
                      "WHERE class_id = ? AND sequence_number > ? ORDER BY sequence_number";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return events;
    }

    sqlite3_bind_int(stmt, 1, class_id);
    sqlite3_bind_int(stmt, 2, sequence_number);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        EventInfo info;
        info.id = sqlite3_column_int(stmt, 0);
        info.event_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        info.class_id = sqlite3_column_int(stmt, 2);
        info.device_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        info.event_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        info.event_data = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        info.signature = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        info.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        info.sequence_number = sqlite3_column_int(stmt, 8);
        events.push_back(info);
    }

    sqlite3_finalize(stmt);
    return events;
}

std::optional<EventInfo> Database::getEventById(const std::string& event_id) {
    std::string sql = "SELECT id, event_id, class_id, device_id, event_type, event_data, signature, timestamp, sequence_number FROM events "
                      "WHERE event_id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, event_id.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        EventInfo info;
        info.id = sqlite3_column_int(stmt, 0);
        info.event_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        info.class_id = sqlite3_column_int(stmt, 2);
        info.device_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        info.event_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        info.event_data = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        info.signature = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        info.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        info.sequence_number = sqlite3_column_int(stmt, 8);
        sqlite3_finalize(stmt);
        return info;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

int Database::getCurrentSequence(int class_id) {
    std::string sql = "SELECT current_sequence FROM sequence_counters WHERE class_id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_int(stmt, 1, class_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int seq = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return seq;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int Database::getNextSequence(int class_id) {
    // First ensure counter exists
    std::string sql = "INSERT OR IGNORE INTO sequence_counters (class_id, current_sequence) VALUES (" + 
                      std::to_string(class_id) + ", 0)";
    executeSql(sql);

    // Get and increment
    sql = "UPDATE sequence_counters SET current_sequence = current_sequence + 1, updated_at = CURRENT_TIMESTAMP "
          "WHERE class_id = " + std::to_string(class_id);
    executeSql(sql);

    return getCurrentSequence(class_id);
}

// Snapshot operations
bool Database::createSnapshot(int class_id, int snapshot_version, const std::string& snapshot_data,
                              const std::string& checksum) {
    std::string snapshot_id = "snap_" + std::to_string(class_id) + "_v" + std::to_string(snapshot_version);
    std::string sql = "INSERT INTO snapshots (snapshot_id, class_id, snapshot_version, snapshot_data, checksum) "
                      "VALUES ('" + snapshot_id + "', " + std::to_string(class_id) + ", " + 
                      std::to_string(snapshot_version) + ", '" + snapshot_data + "', '" + checksum + "')";
    return executeSql(sql);
}

std::optional<SnapshotInfo> Database::getLatestSnapshot(int class_id) {
    std::string sql = "SELECT id, snapshot_id, class_id, snapshot_version, snapshot_data, checksum, is_valid FROM snapshots "
                      "WHERE class_id = ? ORDER BY snapshot_version DESC LIMIT 1";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, class_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        SnapshotInfo info;
        info.id = sqlite3_column_int(stmt, 0);
        info.snapshot_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        info.class_id = sqlite3_column_int(stmt, 2);
        info.snapshot_version = sqlite3_column_int(stmt, 3);
        info.snapshot_data = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        info.checksum = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        info.is_valid = sqlite3_column_int(stmt, 6) != 0;
        sqlite3_finalize(stmt);
        return info;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<SnapshotInfo> Database::getAllSnapshots(int class_id) {
    std::vector<SnapshotInfo> snapshots;
    std::string sql = "SELECT id, snapshot_id, class_id, snapshot_version, snapshot_data, checksum, is_valid FROM snapshots "
                      "WHERE class_id = ? ORDER BY snapshot_version DESC";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return snapshots;
    }

    sqlite3_bind_int(stmt, 1, class_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SnapshotInfo info;
        info.id = sqlite3_column_int(stmt, 0);
        info.snapshot_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        info.class_id = sqlite3_column_int(stmt, 2);
        info.snapshot_version = sqlite3_column_int(stmt, 3);
        info.snapshot_data = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        info.checksum = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        info.is_valid = sqlite3_column_int(stmt, 6) != 0;
        snapshots.push_back(info);
    }

    sqlite3_finalize(stmt);
    return snapshots;
}

bool Database::markSnapshotInvalid(const std::string& snapshot_id) {
    std::string sql = "UPDATE snapshots SET is_valid = FALSE WHERE snapshot_id = '" + snapshot_id + "'";
    return executeSql(sql);
}

// Conflict operations
bool Database::createConflict(const std::string& conflict_id, int class_id, const std::string& conflict_type,
                              const std::string& conflicting_events) {
    std::string sql = "INSERT INTO conflicts (conflict_id, class_id, conflict_type, conflicting_events) "
                      "VALUES ('" + conflict_id + "', " + std::to_string(class_id) + ", '" + 
                      conflict_type + "', '" + conflicting_events + "')";
    return executeSql(sql);
}

std::vector<ConflictInfo> Database::getPendingConflicts(int class_id) {
    std::vector<ConflictInfo> conflicts;
    std::string sql = "SELECT id, conflict_id, class_id, conflict_type, conflicting_events, resolution_status, resolution_data, resolved_by FROM conflicts "
                      "WHERE class_id = ? AND resolution_status = 'pending' ORDER BY created_at";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return conflicts;
    }

    sqlite3_bind_int(stmt, 1, class_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ConflictInfo info;
        info.id = sqlite3_column_int(stmt, 0);
        info.conflict_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        info.class_id = sqlite3_column_int(stmt, 2);
        info.conflict_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        info.conflicting_events = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        info.resolution_status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        info.resolution_data = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        info.resolved_by = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        conflicts.push_back(info);
    }

    sqlite3_finalize(stmt);
    return conflicts;
}

bool Database::resolveConflict(const std::string& conflict_id, const std::string& resolution_data,
                               const std::string& resolved_by) {
    std::string sql = "UPDATE conflicts SET resolution_status = 'resolved', resolution_data = '" + 
                      resolution_data + "', resolved_by = '" + resolved_by + 
                      "', resolved_at = CURRENT_TIMESTAMP WHERE conflict_id = '" + conflict_id + "'";
    return executeSql(sql);
}

// Audit log operations
bool Database::logAuditAction(const std::string& action_type, const std::string& actor_id,
                              const std::string& target_type, const std::string& target_id,
                              const std::string& action_data, const std::string& ip_address) {
    std::string sql = "INSERT INTO audit_log (action_type, actor_id, target_type, target_id, action_data, ip_address) "
                      "VALUES ('" + action_type + "', '" + actor_id + "', '" + target_type + "', '" + 
                      target_id + "', '" + action_data + "', '" + ip_address + "')";
    return executeSql(sql);
}

} // namespace server
} // namespace turtleclass
