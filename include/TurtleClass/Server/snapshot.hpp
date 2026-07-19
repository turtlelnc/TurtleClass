#pragma once

#include "TurtleClass/Core/domain.hpp"
#include "TurtleClass/Server/accounts.hpp"

#include <string>
#include <vector>
#include <optional>
#include <map>
#include <filesystem>
#include <cstdint>

namespace turtleclass::server {

using turtleclass::core::StudentId;
using turtleclass::core::StudentState;
using turtleclass::core::RuleSet;
using turtleclass::core::DeviceId;

struct ConflictRecord {
    std::string conflict_id;
    StudentId student_id;
    std::int64_t detected_at_unix = 0;
    std::vector<DeviceId> devices_involved;
    std::vector<std::string> divergent_event_ids;
    bool resolved = false;
    std::optional<std::string> resolution_note;
};

struct SnapshotData {
    int version = 1;
    std::int64_t created_at_unix = 0;
    std::int64_t last_server_sequence = 0;
    RuleSet rule_set;
    std::map<StudentId, StudentState> students;
    std::vector<DeviceEntry> devices;
    std::vector<ConflictRecord> conflicts;
    std::string checksum;  // blake3 hex
};

class SnapshotManager {
public:
    explicit SnapshotManager(std::filesystem::path data_directory);
    
    [[nodiscard]] const std::filesystem::path& data_directory() const noexcept;
    
    // Create snapshot from current state
    SnapshotData create_snapshot(
        std::int64_t last_server_sequence,
        const RuleSet& rules,
        const std::map<StudentId, StudentState>& students,
        const std::vector<DeviceEntry>& devices,
        const std::vector<ConflictRecord>& conflicts
    ) const;
    
    // Save snapshot to disk
    void save_snapshot(const SnapshotData& snapshot) const;
    
    // Load latest snapshot from disk
    [[nodiscard]] std::optional<SnapshotData> load_snapshot() const;
    
    // Create rolling backup
    void create_rolling_backup(std::size_t max_backups = 5) const;
    
    // Verify snapshot integrity
    [[nodiscard]] bool verify_checksum(const SnapshotData& snapshot) const;
    
    // Compute checksum for snapshot data (excluding checksum field)
    [[nodiscard]] std::string compute_checksum(const SnapshotData& snapshot) const;

private:
    std::filesystem::path data_directory_;
    [[nodiscard]] std::filesystem::path snapshot_path() const;
    [[nodiscard]] std::filesystem::path backup_directory() const;
};

class ConflictDetector {
public:
    // Detect conflicts when uploading events
    // Returns list of newly detected conflicts
    [[nodiscard]] std::vector<ConflictRecord> detect_conflicts(
        const std::vector<ConflictRecord>& existing_conflicts,
        const std::map<StudentId, std::vector<std::string>>& student_events_by_device
    ) const;
    
    // Check if a student is frozen due to conflict
    [[nodiscard]] bool is_student_frozen(
        const StudentId& student_id,
        const std::vector<ConflictRecord>& conflicts
    ) const;
    
    // Resolve a conflict
    bool resolve_conflict(
        std::vector<ConflictRecord>& conflicts,
        const std::string& conflict_id,
        const std::string& keep_event_id,
        const std::vector<std::string>& discard_event_ids,
        const std::string& admin_note
    );
    
    // Generate unique conflict ID
    [[nodiscard]] static std::string generate_conflict_id();
};

} // namespace turtleclass::server
