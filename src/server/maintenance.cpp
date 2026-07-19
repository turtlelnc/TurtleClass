#include "maintenance.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace turtleclass {

const std::set<std::string> MaintenanceMode::always_allowed_endpoints_ = {
    "/health",
    "/api/v1/auth/admin/login",
    "/api/v1/maintenance/status",
    "/api/v1/maintenance/disable"
};

MaintenanceMode& MaintenanceMode::instance() {
    static MaintenanceMode instance;
    return instance;
}

MaintenanceMode::MaintenanceMode() : enabled_(false) {
    // 启动时检查是否存在维护标志文件
    std::string reason, enabled_by;
    if (read_flag_file(reason, enabled_by)) {
        enabled_ = true;
        reason_ = reason;
        enabled_by_ = enabled_by;
        std::cout << "[Maintenance] System started in maintenance mode. Reason: " << reason << std::endl;
    }
}

MaintenanceMode::~MaintenanceMode() {
    // 不自动禁用维护模式，需要管理员手动操作
}

std::filesystem::path MaintenanceMode::get_flag_file_path() const {
    // 维护标志文件路径：/var/lib/turtleclass/maintenance.flag 或 ./data/maintenance.flag
    auto data_dir = std::getenv("TURTLECLASS_DATA_DIR");
    if (data_dir) {
        return std::filesystem::path(data_dir) / "maintenance.flag";
    }
    return std::filesystem::path("./data/maintenance.flag");
}

bool MaintenanceMode::write_flag_file(const std::string& reason, const std::string& enabled_by) {
    try {
        nlohmann::json flag_data;
        flag_data["enabled"] = true;
        flag_data["reason"] = reason;
        flag_data["enabled_by"] = enabled_by;
        flag_data["timestamp"] = std::time(nullptr);
        
        auto flag_path = get_flag_file_path();
        std::filesystem::create_directories(flag_path.parent_path());
        
        std::ofstream file(flag_path);
        if (!file.is_open()) {
            std::cerr << "[Maintenance] Failed to open flag file for writing: " << flag_path << std::endl;
            return false;
        }
        
        file << flag_data.dump(2);
        file.close();
        
        std::cout << "[Maintenance] Flag file written: " << flag_path << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[Maintenance] Error writing flag file: " << e.what() << std::endl;
        return false;
    }
}

bool MaintenanceMode::read_flag_file(std::string& reason, std::string& enabled_by) const {
    try {
        auto flag_path = get_flag_file_path();
        if (!std::filesystem::exists(flag_path)) {
            return false;
        }
        
        std::ifstream file(flag_path);
        if (!file.is_open()) {
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        
        auto flag_data = nlohmann::json::parse(buffer.str());
        
        if (flag_data.value("enabled", false)) {
            reason = flag_data.value("reason", "");
            enabled_by = flag_data.value("enabled_by", "");
            return true;
        }
        
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[Maintenance] Error reading flag file: " << e.what() << std::endl;
        return false;
    }
}

bool MaintenanceMode::delete_flag_file() {
    try {
        auto flag_path = get_flag_file_path();
        if (std::filesystem::exists(flag_path)) {
            std::filesystem::remove(flag_path);
            std::cout << "[Maintenance] Flag file deleted: " << flag_path << std::endl;
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[Maintenance] Error deleting flag file: " << e.what() << std::endl;
        return false;
    }
}

bool MaintenanceMode::enable(const std::string& admin_id, const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (enabled_) {
        std::cerr << "[Maintenance] Already in maintenance mode." << std::endl;
        return false;
    }
    
    if (write_flag_file(reason, admin_id)) {
        enabled_ = true;
        reason_ = reason;
        enabled_by_ = admin_id;
        std::cout << "[Maintenance] Enabled by admin " << admin_id << ". Reason: " << reason << std::endl;
        return true;
    }
    
    return false;
}

bool MaintenanceMode::disable(const std::string& admin_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!enabled_) {
        std::cerr << "[Maintenance] Not in maintenance mode." << std::endl;
        return false;
    }
    
    // 验证是否是启用维护的管理员在禁用（或者任何管理员都可以）
    if (admin_id != enabled_by_) {
        std::cout << "[Maintenance] Warning: Different admin (" << admin_id 
                  << ") disabling maintenance enabled by (" << enabled_by_ << ")" << std::endl;
    }
    
    if (delete_flag_file()) {
        enabled_ = false;
        reason_.clear();
        enabled_by_.clear();
        std::cout << "[Maintenance] Disabled by admin " << admin_id << std::endl;
        return true;
    }
    
    return false;
}

bool MaintenanceMode::is_enabled() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return enabled_;
}

bool MaintenanceMode::is_request_allowed(const std::string& endpoint, const std::string& device_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 如果不在维护模式，所有请求都允许
    if (!enabled_) {
        return true;
    }
    
    // 检查是否在白名单中
    if (whitelist_.count(device_id) > 0) {
        return true;
    }
    
    // 检查端点是否始终允许
    if (always_allowed_endpoints_.count(endpoint) > 0) {
        return true;
    }
    
    // 维护模式下拒绝其他所有请求
    return false;
}

void MaintenanceMode::add_to_whitelist(const std::string& device_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    whitelist_.insert(device_id);
    std::cout << "[Maintenance] Added device to whitelist: " << device_id << std::endl;
}

void MaintenanceMode::remove_from_whitelist(const std::string& device_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    whitelist_.erase(device_id);
    std::cout << "[Maintenance] Removed device from whitelist: " << device_id << std::endl;
}

std::string MaintenanceMode::get_status() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return enabled_ ? "enabled" : "disabled";
}

std::string MaintenanceMode::get_reason() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return reason_;
}

std::string MaintenanceMode::get_enabled_by() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return enabled_by_;
}

} // namespace turtleclass
