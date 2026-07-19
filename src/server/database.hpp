#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include "sqlite3.h"

namespace turtleclass {
namespace server {

struct ClassInfo {
    int id;
    std::string class_code;
    std::string class_name;
    std::string admin_password_hash;
    std::string admin_salt;
};

struct StudentInfo {
    int id;
    int class_id;
    std::string student_id;
    std::string student_name;
    std::string password_hash;
    std::string password_salt;
};

struct DeviceInfo {
    int id;
    std::string device_id;
    std::string device_name;
    std::string device_type;
    std::optional<int> class_id;
    std::string public_key;
    std::string status; // active, revoked, suspended
    std::string last_seen;
};

struct EventInfo {
    int id;
    std::string event_id;
    int class_id;
    std::string device_id;
    std::string event_type;
    std::string event_data;
    std::string signature;
    std::string timestamp;
    int sequence_number;
};

struct SnapshotInfo {
    int id;
    std::string snapshot_id;
    int class_id;
    int snapshot_version;
    std::string snapshot_data;
    std::string checksum;
    bool is_valid;
};

struct ConflictInfo {
    int id;
    std::string conflict_id;
    int class_id;
    std::string conflict_type;
    std::string conflicting_events;
    std::string resolution_status;
    std::string resolution_data;
    std::string resolved_by;
};

class Database {
public:
    Database();
    ~Database();

    bool initialize(const std::string& db_path);
    void close();

    // Class operations
    bool createClass(const std::string& class_code, const std::string& class_name,
                     const std::string& admin_password_hash, const std::string& admin_salt);
    std::optional<ClassInfo> getClassByCode(const std::string& class_code);
    bool updateClass(int class_id, const std::string& class_name);

    // Student operations
    bool addStudent(int class_id, const std::string& student_id, const std::string& student_name,
                    const std::string& password_hash = "", const std::string& password_salt = "");
    std::optional<StudentInfo> getStudent(int class_id, const std::string& student_id);
    std::vector<StudentInfo> getAllStudents(int class_id);
    bool updateStudent(int class_id, const std::string& student_id, const std::string& student_name);
    bool removeStudent(int class_id, const std::string& student_id);

    // Device operations
    bool registerDevice(const std::string& device_id, const std::string& device_name,
                        const std::string& device_type, int class_id, const std::string& public_key);
    std::optional<DeviceInfo> getDevice(const std::string& device_id);
    std::vector<DeviceInfo> getAllDevices(int class_id);
    bool revokeDevice(const std::string& device_id, const std::string& reason, const std::string& revoked_by);
    bool updateDeviceLastSeen(const std::string& device_id);

    // Event operations
    bool insertEvent(const std::string& event_id, int class_id, const std::string& device_id,
                     const std::string& event_type, const std::string& event_data,
                     const std::string& signature, const std::string& timestamp, int sequence_number);
    std::vector<EventInfo> getEventsAfter(int class_id, int sequence_number);
    std::optional<EventInfo> getEventById(const std::string& event_id);
    int getCurrentSequence(int class_id);
    int getNextSequence(int class_id);

    // Snapshot operations
    bool createSnapshot(int class_id, int snapshot_version, const std::string& snapshot_data,
                        const std::string& checksum);
    std::optional<SnapshotInfo> getLatestSnapshot(int class_id);
    std::vector<SnapshotInfo> getAllSnapshots(int class_id);
    bool markSnapshotInvalid(const std::string& snapshot_id);

    // Conflict operations
    bool createConflict(const std::string& conflict_id, int class_id, const std::string& conflict_type,
                        const std::string& conflicting_events);
    std::vector<ConflictInfo> getPendingConflicts(int class_id);
    bool resolveConflict(const std::string& conflict_id, const std::string& resolution_data,
                         const std::string& resolved_by);

    // Audit log operations
    bool logAuditAction(const std::string& action_type, const std::string& actor_id,
                        const std::string& target_type, const std::string& target_id,
                        const std::string& action_data, const std::string& ip_address = "");

private:
    sqlite3* db_;
    bool executeSql(const std::string& sql);
    bool tableExists(const std::string& table_name);
};

} // namespace server
} // namespace turtleclass
