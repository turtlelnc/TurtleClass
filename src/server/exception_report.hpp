#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <map>

namespace turtleclass {

/**
 * 客户端异常报告
 */
struct ExceptionReport {
    std::string device_id;
    std::string exception_type;
    std::string message;
    std::string stack_trace;
    std::string app_version;
    std::string os_version;
    int64_t timestamp;
    std::string class_code;  // 可选：关联的班级代码
};

/**
 * 异常报告聚合统计
 */
struct ExceptionStats {
    int total_count;
    int unique_exceptions;
    std::string most_common_type;
    int affected_devices;
    std::chrono::system_clock::time_point first_occurrence;
    std::chrono::system_clock::time_point last_occurrence;
};

/**
 * 异常报告分发系统
 * 
 * 功能：
 * - 收集客户端异常报告
 * - 聚合分析异常模式
 * - 向其他客户端分发警告
 * - 生成异常趋势统计
 */
class ExceptionReportService {
public:
    static ExceptionReportService& instance();
    
    // 提交异常报告
    bool submit_report(const ExceptionReport& report);
    
    // 获取待分发的异常警告（给其他客户端）
    std::vector<ExceptionReport> get_pending_warnings(const std::string& device_id, int limit = 10);
    
    // 标记异常为已读/已处理
    void mark_as_handled(const std::string& report_id);
    
    // 获取异常统计信息
    ExceptionStats get_stats(const std::string& class_code = "", 
                            const std::string& time_range = "24h") const;
    
    // 获取最常见的异常类型
    std::vector<std::pair<std::string, int>> get_top_exception_types(int limit = 5) const;
    
    // 获取受影响的设备列表
    std::vector<std::string> get_affected_devices(const std::string& exception_type) const;
    
    // 清除旧异常报告（保留最近 N 天）
    int cleanup_old_reports(int days_to_keep = 30);
    
private:
    ExceptionReportService();
    ~ExceptionReportService();
    
    std::string generate_report_id() const;
    std::string hash_exception_signature(const ExceptionReport& report) const;
    
    mutable std::mutex mutex_;
    std::vector<ExceptionReport> reports_;
    std::map<std::string, bool> handled_reports_;  // report_id -> handled
    std::map<std::string, std::vector<std::string>> device_warnings_;  // device_id -> [report_ids]
};

} // namespace turtleclass
