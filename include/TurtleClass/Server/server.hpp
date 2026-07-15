#pragma once

#include "TurtleClass/Core/domain.hpp"

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace turtleclass::server {

using turtleclass::core::ClassId;
using turtleclass::core::DeviceId;
using turtleclass::core::DomainEvent;
using turtleclass::core::EventGroup;
using turtleclass::core::EventId;

struct ServerEventRecord {
    std::int64_t server_sequence = 0;
    std::int64_t server_time_unix = 0;
    DomainEvent event;
};

struct UploadResult {
    bool accepted = false;
    bool duplicate = false;
    std::int64_t first_server_sequence = 0;
    std::int64_t last_server_sequence = 0;
    std::vector<std::string> errors;
};

struct ServerHealth {
    bool ok = true;
    bool maintenance_mode = false;
    std::size_t confirmed_events = 0;
    std::int64_t last_server_sequence = 0;
    std::vector<std::string> errors;
};

class FileEventLog {
public:
    explicit FileEventLog(std::filesystem::path data_directory);

    [[nodiscard]] const std::filesystem::path& data_directory() const noexcept;
    [[nodiscard]] std::vector<ServerEventRecord> load() const;
    void append(const std::vector<ServerEventRecord>& records);
    void export_to(const std::filesystem::path& output_file) const;
    void create_rolling_backup(std::size_t max_backups = 5) const;

private:
    std::filesystem::path data_directory_;
    [[nodiscard]] std::filesystem::path event_log_path() const;
    [[nodiscard]] std::filesystem::path backup_directory() const;
};

class ServerBackend {
public:
    ServerBackend(ClassId class_id, FileEventLog storage);

    void load_from_disk();
    [[nodiscard]] UploadResult upload(const EventGroup& group);
    [[nodiscard]] std::vector<ServerEventRecord> download_after(std::int64_t server_sequence) const;
    [[nodiscard]] ServerHealth health() const;
    void set_maintenance_mode(bool enabled) noexcept;
    void create_backup(std::size_t max_backups = 5) const;
    void export_events(const std::filesystem::path& output_file) const;

private:
    ClassId class_id_;
    FileEventLog storage_;
    bool maintenance_mode_ = false;
    std::int64_t next_server_sequence_ = 1;
    std::vector<ServerEventRecord> records_;
    std::map<EventId, std::int64_t> event_sequences_;
    std::map<DeviceId, std::int64_t> last_device_sequences_;

    [[nodiscard]] std::vector<std::string> validate_upload(const EventGroup& group) const;
    void index_record(const ServerEventRecord& record);
};

} // namespace turtleclass::server
