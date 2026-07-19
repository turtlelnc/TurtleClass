#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace turtleclass::windows_desktop {

// Event structure matching server format
struct SyncEvent {
    std::string event_id;
    int64_t client_sequence;
    int64_t timestamp;
    std::string event_type;  // "points_change", "badge_awarded", etc.
    std::string student_id;
    int points_delta;
    std::string reason;
    std::string device_id;
    std::string signature;  // Ed25519 signature
};

struct SyncState {
    int64_t last_synced_sequence = 0;
    int64_t pending_upload_count = 0;
    bool is_syncing = false;
    std::wstring last_error;
    std::wstring last_sync_time;
};

enum class SyncDirection {
    UploadOnly,
    DownloadOnly,
    Bidirectional
};

class SyncService {
public:
    SyncService();
    
    void initialize(const std::wstring& server_url, const std::wstring& device_token);
    
    // Queue event for later sync
    void queue_event(const SyncEvent& event);
    
    // Perform sync operation
    [[nodiscard]] bool sync(SyncDirection direction = SyncDirection::Bidirectional);
    
    // Get pending events count
    [[nodiscard]] size_t get_pending_events_count() const;
    
    // Get sync state
    [[nodiscard]] SyncState get_sync_state() const;
    
    // Clear error state
    void clear_error();
    
private:
    std::wstring server_url_;
    std::wstring device_token_;
    std::vector<SyncEvent> pending_events_;
    SyncState sync_state_;
    
    bool upload_pending_events();
    bool download_new_events();
    void update_sync_state();
};

} // namespace turtleclass::windows_desktop
