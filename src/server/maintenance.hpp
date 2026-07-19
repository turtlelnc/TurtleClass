#pragma once

#include <string>
#include <filesystem>
#include <set>
#include <mutex>

namespace turtleclass {

/**
 * 维护模式管理系统
 * 
 * 功能：
 * - 启用/禁用维护模式
 * - 检查 API 请求是否被允许
 * - 管理紧急访问白名单
 */
class MaintenanceMode {
public:
    static MaintenanceMode& instance();
    
    // 启用维护模式
    bool enable(const std::string& admin_id, const std::string& reason);
    
    // 禁用维护模式
    bool disable(const std::string& admin_id);
    
    // 检查是否处于维护模式
    bool is_enabled() const;
    
    // 检查请求是否被允许（白名单）
    bool is_request_allowed(const std::string& endpoint, const std::string& device_id) const;
    
    // 添加到紧急访问白名单
    void add_to_whitelist(const std::string& device_id);
    
    // 从白名单移除
    void remove_from_whitelist(const std::string& device_id);
    
    // 获取当前维护状态信息
    std::string get_status() const;
    
    // 获取维护原因
    std::string get_reason() const;
    
    // 获取启用维护的管理员 ID
    std::string get_enabled_by() const;
    
private:
    MaintenanceMode();
    ~MaintenanceMode();
    
    std::filesystem::path get_flag_file_path() const;
    bool write_flag_file(const std::string& reason, const std::string& enabled_by);
    bool read_flag_file(std::string& reason, std::string& enabled_by) const;
    bool delete_flag_file();
    
    mutable std::mutex mutex_;
    bool enabled_;
    std::string reason_;
    std::string enabled_by_;
    std::set<std::string> whitelist_;
    
    // 始终允许的端点（即使在维护模式下）
    static const std::set<std::string> always_allowed_endpoints_;
};

} // namespace turtleclass
