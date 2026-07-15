#include "TurtleClass/Server/server.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace turtleclass::server {
namespace {
std::int64_t now_unix() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string encode(const std::string& value) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (unsigned char ch : value) out << std::setw(2) << static_cast<int>(ch);
    return out.str();
}

std::string decode(const std::string& hex) {
    if (hex.size() % 2 != 0) throw std::runtime_error("corrupt hex field");
    std::string value;
    value.reserve(hex.size() / 2);
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        value.push_back(static_cast<char>(std::stoi(hex.substr(i, 2), nullptr, 16)));
    }
    return value;
}

std::vector<std::string> split_tab(const std::string& line) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= line.size()) {
        const auto end = line.find('\t', start);
        if (end == std::string::npos) {
            parts.push_back(line.substr(start));
            break;
        }
        parts.push_back(line.substr(start, end - start));
        start = end + 1;
    }
    return parts;
}
} // namespace

FileEventLog::FileEventLog(std::filesystem::path data_directory) : data_directory_(std::move(data_directory)) {}
const std::filesystem::path& FileEventLog::data_directory() const noexcept { return data_directory_; }
std::filesystem::path FileEventLog::event_log_path() const { return data_directory_ / "events.tsv"; }
std::filesystem::path FileEventLog::backup_directory() const { return data_directory_ / "backups"; }

std::vector<ServerEventRecord> FileEventLog::load() const {
    std::vector<ServerEventRecord> records;
    std::ifstream in(event_log_path());
    if (!in) return records;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto parts = split_tab(line);
        if (parts.size() != 14) throw std::runtime_error("corrupt event log line");
        ServerEventRecord record;
        record.server_sequence = std::stoll(parts[0]);
        record.server_time_unix = std::stoll(parts[1]);
        record.event.event_id = EventId{decode(parts[2])};
        record.event.class_id = ClassId{decode(parts[3])};
        record.event.event_group_id = turtleclass::core::EventGroupId{decode(parts[4])};
        record.event.target_id = turtleclass::core::StudentId{decode(parts[5])};
        record.event.device_id = DeviceId{decode(parts[6])};
        record.event.device_local_sequence = std::stoll(parts[7]);
        record.event.event_type = static_cast<turtleclass::core::EventType>(std::stoi(parts[8]));
        record.event.points_delta = std::stoi(parts[9]);
        record.event.badge_delta = std::stoi(parts[10]);
        record.event.rule_version = std::stoi(parts[11]);
        record.event.previous_hash = decode(parts[12]);
        auto compensated = decode(parts[13]);
        if (!compensated.empty()) record.event.compensates_event_id = EventId{compensated};
        records.push_back(record);
    }
    return records;
}

void FileEventLog::append(const std::vector<ServerEventRecord>& records) {
    std::filesystem::create_directories(data_directory_);
    std::ofstream out(event_log_path(), std::ios::app);
    if (!out) throw std::runtime_error("cannot open event log for append");
    for (const auto& record : records) {
        out << record.server_sequence << '\t'
            << record.server_time_unix << '\t'
            << encode(record.event.event_id.value()) << '\t'
            << encode(record.event.class_id.value()) << '\t'
            << encode(record.event.event_group_id.value()) << '\t'
            << encode(record.event.target_id.value()) << '\t'
            << encode(record.event.device_id.value()) << '\t'
            << record.event.device_local_sequence << '\t'
            << static_cast<int>(record.event.event_type) << '\t'
            << record.event.points_delta << '\t'
            << record.event.badge_delta << '\t'
            << record.event.rule_version << '\t'
            << encode(record.event.previous_hash) << '\t'
            << encode(record.event.compensates_event_id ? record.event.compensates_event_id->value() : std::string{}) << '\n';
    }
}

void FileEventLog::export_to(const std::filesystem::path& output_file) const {
    std::filesystem::create_directories(output_file.parent_path());
    std::filesystem::copy_file(event_log_path(), output_file, std::filesystem::copy_options::overwrite_existing);
}

void FileEventLog::create_rolling_backup(std::size_t max_backups) const {
    std::filesystem::create_directories(backup_directory());
    if (!std::filesystem::exists(event_log_path())) return;
    const auto backup = backup_directory() / ("events-" + std::to_string(now_unix()) + ".tsv");
    std::filesystem::copy_file(event_log_path(), backup, std::filesystem::copy_options::overwrite_existing);
    std::vector<std::filesystem::directory_entry> entries;
    for (const auto& entry : std::filesystem::directory_iterator(backup_directory())) if (entry.is_regular_file()) entries.push_back(entry);
    std::ranges::sort(entries, [](const auto& a, const auto& b) { return a.last_write_time() > b.last_write_time(); });
    for (std::size_t i = max_backups; i < entries.size(); ++i) std::filesystem::remove(entries[i]);
}

ServerBackend::ServerBackend(ClassId class_id, FileEventLog storage) : class_id_(std::move(class_id)), storage_(std::move(storage)) {}

void ServerBackend::load_from_disk() {
    records_ = storage_.load();
    event_sequences_.clear();
    last_device_sequences_.clear();
    next_server_sequence_ = 1;
    for (const auto& record : records_) index_record(record);
}

UploadResult ServerBackend::upload(const EventGroup& group) {
    UploadResult result;
    if (maintenance_mode_) {
        result.errors.push_back("server is in maintenance mode");
        return result;
    }
    if (!group.events.empty() && event_sequences_.contains(group.events.front().event_id)) {
        result.accepted = true;
        result.duplicate = true;
        result.first_server_sequence = event_sequences_.at(group.events.front().event_id);
        result.last_server_sequence = result.first_server_sequence;
        return result;
    }
    result.errors = validate_upload(group);
    if (!result.errors.empty()) return result;

    std::vector<ServerEventRecord> accepted;
    accepted.reserve(group.events.size());
    const auto server_time = now_unix();
    for (const auto& event : group.events) accepted.push_back({next_server_sequence_++, server_time, event});
    storage_.append(accepted);
    for (const auto& record : accepted) { records_.push_back(record); index_record(record); }
    result.accepted = true;
    result.first_server_sequence = accepted.front().server_sequence;
    result.last_server_sequence = accepted.back().server_sequence;
    return result;
}

std::vector<ServerEventRecord> ServerBackend::download_after(std::int64_t server_sequence) const {
    std::vector<ServerEventRecord> out;
    for (const auto& record : records_) if (record.server_sequence > server_sequence) out.push_back(record);
    return out;
}

ServerHealth ServerBackend::health() const {
    ServerHealth health;
    health.maintenance_mode = maintenance_mode_;
    health.confirmed_events = records_.size();
    health.last_server_sequence = next_server_sequence_ - 1;
    if (class_id_.empty()) { health.ok = false; health.errors.push_back("class id is not configured"); }
    return health;
}
void ServerBackend::set_maintenance_mode(bool enabled) noexcept { maintenance_mode_ = enabled; }
void ServerBackend::create_backup(std::size_t max_backups) const { storage_.create_rolling_backup(max_backups); }
void ServerBackend::export_events(const std::filesystem::path& output_file) const { storage_.export_to(output_file); }

std::vector<std::string> ServerBackend::validate_upload(const EventGroup& group) const {
    turtleclass::core::InMemoryEventStore validator;
    auto append = validator.append_group(group);
    std::vector<std::string> errors = append.errors;
    if (group.class_id != class_id_) errors.push_back("event group class id does not match server class id");
    for (const auto& event : group.events) {
        if (event_sequences_.contains(event.event_id)) errors.push_back("event id already committed");
        const auto last = last_device_sequences_.find(event.device_id);
        if (last != last_device_sequences_.end() && event.device_local_sequence <= last->second) errors.push_back("device local sequence is not newer than last accepted sequence");
    }
    return errors;
}

void ServerBackend::index_record(const ServerEventRecord& record) {
    event_sequences_[record.event.event_id] = record.server_sequence;
    auto& last = last_device_sequences_[record.event.device_id];
    last = std::max(last, record.event.device_local_sequence);
    next_server_sequence_ = std::max(next_server_sequence_, record.server_sequence + 1);
}

} // namespace turtleclass::server
