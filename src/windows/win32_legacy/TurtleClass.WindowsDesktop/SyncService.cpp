#include "SyncService.hpp"
#include "HttpClient.hpp"

#include <sstream>
#include <chrono>
#include <ctime>

namespace turtleclass::windows_desktop {

namespace {
    std::wstring get_current_time_string() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        
#ifdef _WIN32
        struct tm timeinfo;
        localtime_s(&timeinfo, &time_t_now);
        wchar_t buffer[64];
        wcsftime(buffer, sizeof(buffer), L"%Y-%m-%d %H:%M:%S", &timeinfo);
        return std::wstring(buffer);
#else
        struct tm* timeinfo = std::localtime(&time_t_now);
        char buffer[64];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
        return std::wstring(buffer, buffer + strlen(buffer));
#endif
    }
    
    std::string wide_to_utf8(const std::wstring& wide) {
        if (wide.empty()) return "";
#ifdef _WIN32
        int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string result(size - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &result[0], size, nullptr, nullptr);
        return result;
#else
        return std::string(wide.begin(), wide.end());
#endif
    }
}

SyncService::SyncService() {
    sync_state_.last_synced_sequence = 0;
    sync_state_.pending_upload_count = 0;
    sync_state_.is_syncing = false;
}

void SyncService::initialize(const std::wstring& server_url, const std::wstring& device_token) {
    server_url_ = server_url;
    device_token_ = device_token;
}

void SyncService::queue_event(const SyncEvent& event) {
    pending_events_.push_back(event);
    sync_state_.pending_upload_count = pending_events_.size();
}

bool SyncService::sync(SyncDirection direction) {
    if (server_url_.empty() || device_token_.empty()) {
        sync_state_.last_error = L"Not initialized. Call initialize() first.";
        return false;
    }
    
    sync_state_.is_syncing = true;
    bool success = true;
    
    HttpClient client(server_url_);
    client.set_auth_token(device_token_);
    
    // Upload phase
    if (direction == SyncDirection::UploadOnly || direction == SyncDirection::Bidirectional) {
        if (!pending_events_.empty()) {
            if (!upload_pending_events()) {
                success = false;
            }
        }
    }
    
    // Download phase
    if (direction == SyncDirection::DownloadOnly || direction == SyncDirection::Bidirectional) {
        if (!download_new_events()) {
            success = false;
        }
    }
    
    sync_state_.is_syncing = false;
    
    if (success) {
        sync_state_.last_sync_time = get_current_time_string();
        sync_state_.last_error.clear();
    }
    
    update_sync_state();
    return success;
}

bool SyncService::upload_pending_events() {
    if (pending_events_.empty()) {
        return true;
    }
    
    // Build JSON array of events
    std::ostringstream json;
    json << "[";
    for (size_t i = 0; i < pending_events_.size(); ++i) {
        if (i > 0) json << ",";
        const auto& event = pending_events_[i];
        json << "{";
        json << "\"event_id\":\"" << event.event_id << "\",";
        json << "\"client_sequence\":" << event.client_sequence << ",";
        json << "\"timestamp\":" << event.timestamp << ",";
        json << "\"event_type\":\"" << event.event_type << "\",";
        json << "\"student_id\":\"" << event.student_id << "\",";
        json << "\"points_delta\":" << event.points_delta << ",";
        json << "\"reason\":\"" << event.reason << "\",";
        json << "\"device_id\":\"" << event.device_id << "\",";
        json << "\"signature\":\"" << event.signature << "\"";
        json << "}";
    }
    json << "]";
    
    HttpClient client(server_url_);
    client.set_auth_token(device_token_);
    
    auto response = client.post(L"/api/v1/events/upload", json.str());
    
    if (response.status_code == 200) {
        // Clear successfully uploaded events
        pending_events_.clear();
        sync_state_.pending_upload_count = 0;
        return true;
    } else if (response.status_code == 409) {
        // Conflict detected - keep events but mark error
        sync_state_.last_error = L"Conflict detected during upload. Some events may conflict with server state.";
        return false;
    } else {
        sync_state_.last_error = L"Upload failed with status: " + std::to_wstring(response.status_code);
        return false;
    }
}

bool SyncService::download_new_events() {
    HttpClient client(server_url_);
    client.set_auth_token(device_token_);
    
    // Request events since last synced sequence
    std::wstring path = L"/api/v1/events/download?since_sequence=" + 
                        std::to_wstring(sync_state_.last_synced_sequence);
    
    auto response = client.get(path);
    
    if (response.status_code == 200) {
        // Parse response and update last_synced_sequence
        // Simplified parsing - use proper JSON parser in production
        std::string body = response.body;
        
        // Look for "last_sequence" in response
        auto seq_pos = body.find("\"last_sequence\"");
        if (seq_pos != std::string::npos) {
            auto colon_pos = body.find(':', seq_pos);
            if (colon_pos != std::string::npos) {
                auto num_start = colon_pos + 1;
                while (num_start < body.size() && (body[num_start] == ' ' || body[num_start] == '\t')) {
                    ++num_start;
                }
                auto num_end = num_start;
                while (num_end < body.size() && (std::isdigit(body[num_end]) || body[num_end] == '-')) {
                    ++num_end;
                }
                if (num_end > num_start) {
                    try {
                        sync_state_.last_synced_sequence = std::stoll(body.substr(num_start, num_end - num_start));
                    } catch (...) {
                        // Keep old value on parse error
                    }
                }
            }
        }
        
        return true;
    } else {
        sync_state_.last_error = L"Download failed with status: " + std::to_wstring(response.status_code);
        return false;
    }
}

size_t SyncService::get_pending_events_count() const {
    return pending_events_.size();
}

SyncState SyncService::get_sync_state() const {
    return sync_state_;
}

void SyncService::clear_error() {
    sync_state_.last_error.clear();
}

void SyncService::update_sync_state() {
    sync_state_.pending_upload_count = pending_events_.size();
}

} // namespace turtleclass::windows_desktop
