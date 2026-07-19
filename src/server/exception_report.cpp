#include "exception_report.hpp"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <random>
#include <set>

namespace turtleclass {

ExceptionReportService& ExceptionReportService::instance() {
    static ExceptionReportService instance;
    return instance;
}

ExceptionReportService::ExceptionReportService() {
    std::cout << "[ExceptionReport] Service initialized" << std::endl;
}

ExceptionReportService::~ExceptionReportService() {
    // 清理资源
}

std::string ExceptionReportService::generate_report_id() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "exc_";
    for (int i = 0; i < 12; i++) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

std::string ExceptionReportService::hash_exception_signature(const ExceptionReport& report) const {
    // 生成异常签名哈希用于去重
    std::stringstream ss;
    ss << report.exception_type << "|" << report.message << "|" << report.app_version;
    return ss.str();  // 简化版本，实际应该使用真正的哈希函数
}

bool ExceptionReportService::submit_report(const ExceptionReport& report) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ExceptionReport new_report = report;
    if (new_report.timestamp == 0) {
        new_report.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
    
    std::string report_id = generate_report_id();
    
    // 检查是否为重复异常（相同签名）
    std::string signature = hash_exception_signature(new_report);
    bool is_duplicate = false;
    for (const auto& existing : reports_) {
        if (hash_exception_signature(existing) == signature &&
            existing.device_id != new_report.device_id) {
            // 相同异常但不同设备，说明是普遍问题
            is_duplicate = true;
            break;
        }
    }
    
    reports_.push_back(new_report);
    handled_reports_[report_id] = false;
    
    // 如果是新类型的异常或影响多个设备，标记为需要分发警告
    if (!is_duplicate) {
        // 向其他设备分发警告
        for (const auto& other_report : reports_) {
            if (other_report.device_id != new_report.device_id && 
                other_report.class_code == new_report.class_code) {
                device_warnings_[other_report.device_id].push_back(report_id);
            }
        }
    }
    
    std::cout << "[ExceptionReport] Received report from device " << new_report.device_id 
              << ": " << new_report.exception_type << std::endl;
    
    return true;
}

std::vector<ExceptionReport> ExceptionReportService::get_pending_warnings(
    const std::string& device_id, int limit) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<ExceptionReport> warnings;
    
    auto it = device_warnings_.find(device_id);
    if (it != device_warnings_.end()) {
        for (const auto& report_id : it->second) {
            if (static_cast<int>(warnings.size()) >= limit) {
                break;
            }
            
            // 查找对应的报告
            for (const auto& report : reports_) {
                // 这里简化处理，实际应该用 report_id 索引
                if (!handled_reports_.count(report_id) || !handled_reports_.at(report_id)) {
                    warnings.push_back(report);
                }
            }
        }
    }
    
    return warnings;
}

void ExceptionReportService::mark_as_handled(const std::string& report_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    handled_reports_[report_id] = true;
}

ExceptionStats ExceptionReportService::get_stats(const std::string& class_code, 
                                                  const std::string& time_range) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ExceptionStats stats;
    stats.total_count = 0;
    stats.unique_exceptions = 0;
    stats.affected_devices = 0;
    
    auto now = std::chrono::system_clock::now();
    std::chrono::hours hours(24);
    
    if (time_range == "24h") {
        hours = std::chrono::hours(24);
    } else if (time_range == "7d") {
        hours = std::chrono::hours(24 * 7);
    } else if (time_range == "30d") {
        hours = std::chrono::hours(24 * 30);
    }
    
    auto cutoff = now - hours;
    
    std::set<std::string> unique_signatures;
    std::set<std::string> devices;
    std::map<std::string, int> type_counts;
    
    for (const auto& report : reports_) {
        // 按班级代码过滤
        if (!class_code.empty() && report.class_code != class_code) {
            continue;
        }
        
        // 按时间过滤
        auto report_time = std::chrono::system_clock::from_time_t(report.timestamp);
        if (report_time < cutoff) {
            continue;
        }
        
        stats.total_count++;
        devices.insert(report.device_id);
        
        std::string sig = hash_exception_signature(report);
        if (unique_signatures.find(sig) == unique_signatures.end()) {
            unique_signatures.insert(sig);
            stats.unique_exceptions++;
        }
        
        type_counts[report.exception_type]++;
        
        if (stats.first_occurrence.time_since_epoch().count() == 0 || 
            report_time < stats.first_occurrence) {
            stats.first_occurrence = report_time;
        }
        
        if (report_time > stats.last_occurrence) {
            stats.last_occurrence = report_time;
        }
    }
    
    stats.affected_devices = static_cast<int>(devices.size());
    
    // 找出最常见的异常类型
    int max_count = 0;
    for (const auto& pair : type_counts) {
        if (pair.second > max_count) {
            max_count = pair.second;
            stats.most_common_type = pair.first;
        }
    }
    
    return stats;
}

std::vector<std::pair<std::string, int>> ExceptionReportService::get_top_exception_types(int limit) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::map<std::string, int> type_counts;
    for (const auto& report : reports_) {
        type_counts[report.exception_type]++;
    }
    
    std::vector<std::pair<std::string, int>> sorted_types(type_counts.begin(), type_counts.end());
    std::sort(sorted_types.begin(), sorted_types.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    if (static_cast<int>(sorted_types.size()) > limit) {
        sorted_types.resize(limit);
    }
    
    return sorted_types;
}

std::vector<std::string> ExceptionReportService::get_affected_devices(
    const std::string& exception_type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::set<std::string> devices;
    for (const auto& report : reports_) {
        if (report.exception_type == exception_type) {
            devices.insert(report.device_id);
        }
    }
    
    return std::vector<std::string>(devices.begin(), devices.end());
}

int ExceptionReportService::cleanup_old_reports(int days_to_keep) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto cutoff = std::chrono::system_clock::now() - std::chrono::hours(24 * days_to_keep);
    
    int removed_count = 0;
    auto it = reports_.begin();
    while (it != reports_.end()) {
        auto report_time = std::chrono::system_clock::from_time_t(it->timestamp);
        if (report_time < cutoff) {
            it = reports_.erase(it);
            removed_count++;
        } else {
            ++it;
        }
    }
    
    std::cout << "[ExceptionReport] Cleaned up " << removed_count 
              << " old reports (older than " << days_to_keep << " days)" << std::endl;
    
    return removed_count;
}

} // namespace turtleclass
