#include "TurtleClass/Server/snapshot.hpp"

#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>

namespace turtleclass::server {

namespace {
std::int64_t now_unix() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// Simple hash-based checksum (placeholder for blake3)
std::string simple_checksum(const std::string& data) {
    std::hash<std::string> hasher;
    auto hash_val = hasher(data);
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << hash_val;
    return "blake3$placeholder$" + ss.str();
}

std::string encode_hex(const std::string& value) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (unsigned char ch : value) out << std::setw(2) << static_cast<int>(ch);
    return out.str();
}

std::string decode_hex(const std::string& hex) {
    if (hex.size() % 2 != 0) throw std::runtime_error("corrupt hex field");
    std::string value;
    value.reserve(hex.size() / 2);
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        value.push_back(static_cast<char>(std::stoi(hex.substr(i, 2), nullptr, 16)));
    }
    return value;
}

std::string escape_json_string(const std::string& s) {
    std::ostringstream out;
    for (char c : s) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: out << c; break;
        }
    }
    return out.str();
}

} // namespace

SnapshotManager::SnapshotManager(std::filesystem::path data_directory)
    : data_directory_(std::move(data_directory)) {}

const std::filesystem::path& SnapshotManager::data_directory() const noexcept {
    return data_directory_;
}

std::filesystem::path SnapshotManager::snapshot_path() const {
    return data_directory_ / "snapshot.json";
}

std::filesystem::path SnapshotManager::backup_directory() const {
    return data_directory_ / "backups";
}

SnapshotData SnapshotManager::create_snapshot(
    std::int64_t last_server_sequence,
    const RuleSet& rules,
    const std::map<StudentId, StudentState>& students,
    const std::vector<DeviceEntry>& devices,
    const std::vector<ConflictRecord>& conflicts
) const {
    SnapshotData snapshot;
    snapshot.version = 1;
    snapshot.created_at_unix = now_unix();
    snapshot.last_server_sequence = last_server_sequence;
    snapshot.rule_set = rules;
    snapshot.students = students;
    snapshot.devices = devices;
    snapshot.conflicts = conflicts;
    snapshot.checksum = compute_checksum(snapshot);
    return snapshot;
}

std::string SnapshotManager::compute_checksum(const SnapshotData& snapshot) const {
    // Serialize snapshot data (excluding checksum) for hashing
    std::ostringstream ss;
    ss << snapshot.version << "|";
    ss << snapshot.created_at_unix << "|";
    ss << snapshot.last_server_sequence << "|";
    ss << snapshot.rule_set.id.value() << "|";
    ss << snapshot.rule_set.version << "|";
    
    // Students
    for (const auto& [id, state] : snapshot.students) {
        ss << id.value() << ":" << state.level_index << ":" 
           << state.points_in_level << ":" << state.badges << ":" 
           << (state.frozen ? "1" : "0") << ";";
    }
    ss << "|";
    
    // Devices
    for (const auto& dev : snapshot.devices) {
        ss << dev.device_id.value() << ":" << dev.display_name << ":" 
           << (dev.active ? "1" : "0") << ";";
    }
    ss << "|";
    
    // Conflicts
    for (const auto& conf : snapshot.conflicts) {
        ss << conf.conflict_id << ":" << conf.student_id.value() << ":" 
           << (conf.resolved ? "1" : "0") << ";";
    }
    
    return simple_checksum(ss.str());
}

void SnapshotManager::save_snapshot(const SnapshotData& snapshot) const {
    std::filesystem::create_directories(data_directory_);
    std::ofstream out(snapshot_path());
    if (!out) throw std::runtime_error("cannot open snapshot file for writing");
    
    // Manual JSON serialization
    out << "{\n";
    out << "  \"version\": " << snapshot.version << ",\n";
    out << "  \"created_at_unix\": " << snapshot.created_at_unix << ",\n";
    out << "  \"last_server_sequence\": " << snapshot.last_server_sequence << ",\n";
    
    // Rule set
    out << "  \"rule_set\": {\n";
    out << "    \"id\": \"" << encode_hex(snapshot.rule_set.id.value()) << "\",\n";
    out << "    \"version\": " << snapshot.rule_set.version << ",\n";
    out << "    \"levels\": [";
    for (std::size_t i = 0; i < snapshot.rule_set.levels.size(); ++i) {
        if (i > 0) out << ", ";
        const auto& level = snapshot.rule_set.levels[i];
        out << "{\"name\": \"" << escape_json_string(level.name) 
            << "\", \"points_to_next\": " << level.points_to_next
            << ", \"badges_on_enter\": " << level.badges_on_enter << "}";
    }
    out << "]\n";
    out << "  },\n";
    
    // Students
    out << "  \"students\": {\n";
    bool first = true;
    for (const auto& [id, state] : snapshot.students) {
        if (!first) out << ",\n";
        first = false;
        out << "    \"" << encode_hex(id.value()) << "\": {";
        out << "\"level_index\": " << state.level_index << ", ";
        out << "\"points_in_level\": " << state.points_in_level << ", ";
        out << "\"badges\": " << state.badges << ", ";
        out << "\"frozen\": " << (state.frozen ? "true" : "false") << "}";
    }
    out << "\n  },\n";
    
    // Devices
    out << "  \"devices\": [\n";
    for (std::size_t i = 0; i < snapshot.devices.size(); ++i) {
        if (i > 0) out << ",\n";
        const auto& dev = snapshot.devices[i];
        out << "    {\"device_id\": \"" << encode_hex(dev.device_id.value()) 
            << "\", \"display_name\": \"" << escape_json_string(dev.display_name) << "\", ";
        out << "\"public_key_hex\": \"" << dev.public_key_hex << "\", ";
        out << "\"registered_at_unix\": " << dev.registered_at_unix << ", ";
        out << "\"active\": " << (dev.active ? "true" : "false") << "}";
    }
    out << "\n  ],\n";
    
    // Conflicts
    out << "  \"conflicts\": [\n";
    for (std::size_t i = 0; i < snapshot.conflicts.size(); ++i) {
        if (i > 0) out << ",\n";
        const auto& conf = snapshot.conflicts[i];
        out << "    {\"conflict_id\": \"" << conf.conflict_id 
            << "\", \"student_id\": \"" << encode_hex(conf.student_id.value()) << "\", ";
        out << "\"detected_at_unix\": " << conf.detected_at_unix << ", ";
        out << "\"resolved\": " << (conf.resolved ? "true" : "false");
        if (conf.resolution_note) {
            out << ", \"resolution_note\": \"" << escape_json_string(*conf.resolution_note) << "\"";
        }
        out << "}";
    }
    out << "\n  ],\n";
    
    // Checksum
    out << "  \"checksum\": \"" << snapshot.checksum << "\"\n";
    out << "}\n";
}

std::optional<SnapshotData> SnapshotManager::load_snapshot() const {
    std::ifstream in(snapshot_path());
    if (!in) return std::nullopt;
    
    // Simple JSON parsing (Phase 2 alpha - use proper library in production)
    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string content = buffer.str();
    
    if (content.empty()) return std::nullopt;
    
    // Placeholder parsing - in production use nlohmann/json or similar
    SnapshotData snapshot;
    snapshot.version = 1;
    snapshot.created_at_unix = now_unix();
    snapshot.last_server_sequence = 0;
    snapshot.rule_set = RuleSet::default_rules();
    snapshot.checksum = compute_checksum(snapshot);
    
    return snapshot;
}

void SnapshotManager::create_rolling_backup(std::size_t max_backups) const {
    std::filesystem::create_directories(backup_directory());
    if (!std::filesystem::exists(snapshot_path())) return;
    
    const auto backup = backup_directory() / ("snapshot-" + std::to_string(now_unix()) + ".json");
    std::filesystem::copy_file(snapshot_path(), backup, std::filesystem::copy_options::overwrite_existing);
    
    std::vector<std::filesystem::directory_entry> entries;
    for (const auto& entry : std::filesystem::directory_iterator(backup_directory())) {
        if (entry.is_regular_file()) entries.push_back(entry);
    }
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        return a.last_write_time() > b.last_write_time();
    });
    
    for (std::size_t i = max_backups; i < entries.size(); ++i) {
        std::filesystem::remove(entries[i]);
    }
}

bool SnapshotManager::verify_checksum(const SnapshotData& snapshot) const {
    return compute_checksum(snapshot) == snapshot.checksum;
}

// ConflictDetector implementation

std::vector<ConflictRecord> ConflictDetector::detect_conflicts(
    const std::vector<ConflictRecord>& existing_conflicts,
    const std::map<StudentId, std::vector<std::string>>& student_events_by_device
) const {
    std::vector<ConflictRecord> new_conflicts;
    
    // For each student with events from multiple devices
    for (const auto& [student_id, event_ids] : student_events_by_device) {
        // Check if already has unresolved conflict
        bool has_unresolved = false;
        for (const auto& conf : existing_conflicts) {
            if (conf.student_id == student_id && !conf.resolved) {
                has_unresolved = true;
                break;
            }
        }
        
        if (!has_unresolved && event_ids.size() > 1) {
            // Create new conflict record
            ConflictRecord conflict;
            conflict.conflict_id = generate_conflict_id();
            conflict.student_id = student_id;
            conflict.detected_at_unix = now_unix();
            conflict.divergent_event_ids = event_ids;
            conflict.resolved = false;
            new_conflicts.push_back(conflict);
        }
    }
    
    return new_conflicts;
}

bool ConflictDetector::is_student_frozen(
    const StudentId& student_id,
    const std::vector<ConflictRecord>& conflicts
) const {
    for (const auto& conf : conflicts) {
        if (conf.student_id == student_id && !conf.resolved) {
            return true;
        }
    }
    return false;
}

bool ConflictDetector::resolve_conflict(
    std::vector<ConflictRecord>& conflicts,
    const std::string& conflict_id,
    const std::string& /*keep_event_id*/,
    const std::vector<std::string>& /*discard_event_ids*/,
    const std::string& admin_note
) {
    for (auto& conf : conflicts) {
        if (conf.conflict_id == conflict_id) {
            conf.resolved = true;
            conf.resolution_note = admin_note;
            return true;
        }
    }
    return false;
}

std::string ConflictDetector::generate_conflict_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::string id = "conflict-";
    for (int i = 0; i < 8; ++i) {
        id += "0123456789abcdef"[dis(gen)];
    }
    return id;
}

} // namespace turtleclass::server
