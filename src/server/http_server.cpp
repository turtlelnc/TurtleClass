#include "TurtleClass/Server/http_server.hpp"

#include <sstream>
#include <algorithm>

namespace turtleclass::server {

HttpServer::HttpServer(ServerBackend& backend, AccountStore& accounts,
                       DeviceTable& devices, SnapshotManager& snapshot_mgr)
    : backend_(backend), accounts_(accounts), devices_(devices), snapshot_mgr_(snapshot_mgr) {
    // Register built-in routes
    register_route("GET", "/health", [this](const HttpRequest& req) { return handle_health(req); });
    register_route("POST", "/api/v1/auth/login", [this](const HttpRequest& req) { return handle_login(req); });
    register_route("POST", "/api/v1/devices/register", [this](const HttpRequest& req) { return handle_register_device(req); });
    register_route("POST", "/api/v1/devices/revoke", [this](const HttpRequest& req) { return handle_revoke_device(req); });
    register_route("POST", "/api/v1/events/upload", [this](const HttpRequest& req) { return handle_upload_events(req); });
    register_route("GET", "/api/v1/events/download", [this](const HttpRequest& req) { return handle_download_events(req); });
    register_route("GET", "/api/v1/snapshot", [this](const HttpRequest& req) { return handle_get_snapshot(req); });
    register_route("GET", "/api/v1/conflicts", [this](const HttpRequest& req) { return handle_list_conflicts(req); });
    register_route("POST", "/api/v1/conflicts/resolve", [this](const HttpRequest& req) { return handle_resolve_conflict(req); });
    register_route("GET", "/api/v1/export/events", [this](const HttpRequest& req) { return handle_export_events(req); });
    register_route("GET", "/api/v1/export/logs", [this](const HttpRequest& req) { return handle_export_logs(req); });
}

void HttpServer::register_route(const std::string& method, const std::string& path, HttpHandler handler) {
    routes_[method + " " + path] = std::move(handler);
}

HttpResponse HttpServer::handle_request(const HttpRequest& request) {
    auto it = routes_.find(request.method + " " + request.path);
    if (it != routes_.end()) {
        try {
            return it->second(request);
        } catch (const std::exception& e) {
            return HttpResponse::internal_error(e.what());
        }
    }
    
    // Try pattern matching for paths with parameters
    for (const auto& [route_key, handler] : routes_) {
        std::size_t space_pos = route_key.find(' ');
        if (space_pos == std::string::npos) continue;
        
        std::string route_method = route_key.substr(0, space_pos);
        std::string route_path = route_key.substr(space_pos + 1);
        
        if (route_method != request.method) continue;
        
        // Simple pattern matching for /api/v1/conflicts/{id}/resolve
        if (route_path.find('{') != std::string::npos) {
            // Basic wildcard matching - could be enhanced
            std::string route_prefix = route_path.substr(0, route_path.find('{'));
            if (request.path.rfind(route_prefix, 0) == 0) {
                try {
                    return handler(request);
                } catch (const std::exception& e) {
                    return HttpResponse::internal_error(e.what());
                }
            }
        }
    }
    
    return HttpResponse::not_found("Route not found: " + request.method + " " + request.path);
}

HttpResponse HttpServer::handle_health(const HttpRequest& /*request*/) {
    auto health = backend_.health();
    
    std::ostringstream json;
    json << "{\"ok\": " << (health.ok ? "true" : "false");
    json << ", \"maintenance_mode\": " << (health.maintenance_mode ? "true" : "false");
    json << ", \"confirmed_events\": " << health.confirmed_events;
    json << ", \"active_devices\": " << health.active_devices;
    json << ", \"last_server_sequence\": " << health.last_server_sequence;
    
    if (!health.errors.empty()) {
        json << ", \"errors\": [";
        for (std::size_t i = 0; i < health.errors.size(); ++i) {
            if (i > 0) json << ", ";
            json << "\"" << health.errors[i] << "\"";
        }
        json << "]";
    }
    
    json << "}";
    
    auto response = HttpResponse::ok(json.str());
    response.headers["X-API-Version"] = "v1";
    return response;
}

HttpResponse HttpServer::handle_login(const HttpRequest& request) {
    // Parse JSON body (simplified - use proper JSON parser in production)
    auto class_id = request.get_header("X-Class-Id");
    auto class_token = request.get_header("X-Class-Token");
    auto admin_id = request.get_header("X-Admin-Id");
    auto admin_token = request.get_header("X-Admin-Token");
    
    if (!class_id.empty() && !class_token.empty()) {
        if (accounts_.verify_class_token(AccountId{class_id}, class_token)) {
            return HttpResponse::ok("{\"success\": true, \"type\": \"class\"}");
        }
        return HttpResponse::unauthorized("Invalid class credentials");
    }
    
    if (!admin_id.empty() && !admin_token.empty()) {
        if (accounts_.verify_admin_password(AdminId{admin_id}, admin_token)) {
            return HttpResponse::ok("{\"success\": true, \"type\": \"admin\"}");
        }
        return HttpResponse::unauthorized("Invalid admin credentials");
    }
    
    return HttpResponse::bad_request("Missing credentials");
}

HttpResponse HttpServer::handle_register_device(const HttpRequest& request) {
    // Require admin auth
    auto admin_id = request.get_header("X-Admin-Id");
    auto admin_token = request.get_header("X-Admin-Token");
    
    if (admin_id.empty() || admin_token.empty() || 
        !accounts_.verify_admin_password(AdminId{admin_id}, admin_token)) {
        return HttpResponse::forbidden("Admin authentication required");
    }
    
    // Parse device registration from body (simplified)
    // In production, parse JSON properly
    std::string device_id = "device-" + std::to_string(std::time(nullptr));
    std::string display_name = "Unknown Device";
    std::string public_key = "";
    
    // Extract from body (simplified parsing)
    auto pos = request.body.find("\"device_id\"");
    if (pos != std::string::npos) {
        auto start = request.body.find('"', pos + 11);
        if (start != std::string::npos) {
            auto end = request.body.find('"', start + 1);
            if (end != std::string::npos) {
                device_id = request.body.substr(start + 1, end - start - 1);
            }
        }
    }
    
    pos = request.body.find("\"display_name\"");
    if (pos != std::string::npos) {
        auto start = request.body.find('"', pos + 14);
        if (start != std::string::npos) {
            auto end = request.body.find('"', start + 1);
            if (end != std::string::npos) {
                display_name = request.body.substr(start + 1, end - start - 1);
            }
        }
    }
    
    pos = request.body.find("\"public_key\"");
    if (pos != std::string::npos) {
        auto start = request.body.find('"', pos + 12);
        if (start != std::string::npos) {
            auto end = request.body.find('"', start + 1);
            if (end != std::string::npos) {
                public_key = request.body.substr(start + 1, end - start - 1);
            }
        }
    }
    
    if (devices_.register_device(DeviceId{device_id}, display_name, public_key)) {
        std::ostringstream json;
        json << "{\"success\": true, \"device_id\": \"" << device_id << "\"}";
        return HttpResponse::ok(json.str());
    }
    
    return HttpResponse::bad_request("Failed to register device (may already exist)");
}

HttpResponse HttpServer::handle_revoke_device(const HttpRequest& request) {
    auto admin_id = request.get_header("X-Admin-Id");
    auto admin_token = request.get_header("X-Admin-Token");
    
    if (admin_id.empty() || admin_token.empty() || 
        !accounts_.verify_admin_password(AdminId{admin_id}, admin_token)) {
        return HttpResponse::forbidden("Admin authentication required");
    }
    
    // Parse device_id from body
    std::string device_id;
    auto pos = request.body.find("\"device_id\"");
    if (pos != std::string::npos) {
        auto start = request.body.find('"', pos + 11);
        if (start != std::string::npos) {
            auto end = request.body.find('"', start + 1);
            if (end != std::string::npos) {
                device_id = request.body.substr(start + 1, end - start - 1);
            }
        }
    }
    
    if (device_id.empty()) {
        return HttpResponse::bad_request("Missing device_id");
    }
    
    if (devices_.revoke_device(DeviceId{device_id})) {
        return HttpResponse::ok("{\"success\": true}");
    }
    
    return HttpResponse::bad_request("Device not found");
}

HttpResponse HttpServer::handle_upload_events(const HttpRequest& request) {
    // Verify class auth
    if (!verify_class_auth(request)) {
        return HttpResponse::unauthorized("Class authentication required");
    }
    
    // Parse event group from body (simplified - use proper JSON parser)
    // This is a placeholder - real implementation needs full JSON parsing
    EventGroup group;
    group.group_id = core::EventGroupId{"group-" + std::to_string(std::time(nullptr))};
    group.class_id = ClassId{"class1"};  // Should extract from auth
    
    // Placeholder: create a dummy event
    DomainEvent event;
    event.event_id = EventId{"event-" + std::to_string(std::time(nullptr))};
    event.class_id = group.class_id;
    event.event_group_id = group.group_id;
    event.target_id = StudentId{"student1"};
    event.device_id = DeviceId{"device1"};
    event.device_local_sequence = 1;
    event.event_type = core::EventType::PointsAdjusted;
    event.points_delta = 5;
    event.badge_delta = 0;
    event.rule_version = 1;
    
    group.events.push_back(event);
    
    auto result = backend_.upload(group);
    
    std::ostringstream json;
    json << "{\"accepted\": " << (result.accepted ? "true" : "false");
    json << ", \"duplicate\": " << (result.duplicate ? "true" : "false");
    json << ", \"first_server_sequence\": " << result.first_server_sequence;
    json << ", \"last_server_sequence\": " << result.last_server_sequence;
    
    if (!result.errors.empty()) {
        json << ", \"errors\": [";
        for (std::size_t i = 0; i < result.errors.size(); ++i) {
            if (i > 0) json << ", ";
            json << "\"" << result.errors[i] << "\"";
        }
        json << "]";
    }
    
    json << "}";
    
    if (result.accepted) {
        return HttpResponse::ok(json.str());
    } else {
        return HttpResponse::bad_request(json.str());
    }
}

HttpResponse HttpServer::handle_download_events(const HttpRequest& request) {
    if (!verify_class_auth(request)) {
        return HttpResponse::unauthorized("Class authentication required");
    }
    
    auto after_str = request.query_param("after");
    std::int64_t after_seq = 0;
    if (!after_str.empty()) {
        after_seq = std::stoll(after_str);
    }
    
    auto records = backend_.download_after(after_seq);
    
    std::ostringstream json;
    json << "{\"events\": [";
    for (std::size_t i = 0; i < records.size(); ++i) {
        if (i > 0) json << ", ";
        const auto& record = records[i];
        json << "{\"server_sequence\": " << record.server_sequence;
        json << ", \"event_id\": \"" << record.event.event_id.value() << "\"";
        json << ", \"points_delta\": " << record.event.points_delta;
        json << ", \"badge_delta\": " << record.event.badge_delta;
        json << "}";
    }
    json << "]}";
    
    return HttpResponse::ok(json.str());
}

HttpResponse HttpServer::handle_get_snapshot(const HttpRequest& request) {
    if (!verify_class_auth(request)) {
        return HttpResponse::unauthorized("Class authentication required");
    }
    
    auto snapshot = snapshot_mgr_.load_snapshot();
    if (!snapshot.has_value()) {
        return HttpResponse::not_found("No snapshot available");
    }
    
    // Return snapshot as JSON (simplified)
    std::ostringstream json;
    json << "{\"version\": " << snapshot->version;
    json << ", \"created_at_unix\": " << snapshot->created_at_unix;
    json << ", \"last_server_sequence\": " << snapshot->last_server_sequence;
    json << ", \"checksum\": \"" << snapshot->checksum << "\"}";
    
    return HttpResponse::ok(json.str());
}

HttpResponse HttpServer::handle_list_conflicts(const HttpRequest& request) {
    if (!verify_admin_auth(request)) {
        return HttpResponse::forbidden("Admin authentication required");
    }
    
    // Placeholder - conflicts would be stored and retrieved
    return HttpResponse::ok("{\"conflicts\": []}");
}

HttpResponse HttpServer::handle_resolve_conflict(const HttpRequest& request) {
    if (!verify_admin_auth(request)) {
        return HttpResponse::forbidden("Admin authentication required");
    }
    
    // Placeholder - would resolve conflict based on body
    return HttpResponse::ok("{\"success\": true}");
}

HttpResponse HttpServer::handle_export_events(const HttpRequest& request) {
    if (!verify_admin_auth(request)) {
        return HttpResponse::forbidden("Admin authentication required");
    }
    
    // Trigger export (in production, return file or streaming response)
    return HttpResponse::ok("{\"exported\": true}");
}

HttpResponse HttpServer::handle_export_logs(const HttpRequest& request) {
    if (!verify_admin_auth(request)) {
        return HttpResponse::forbidden("Admin authentication required");
    }
    
    // Trigger log export
    return HttpResponse::ok("{\"exported\": true}");
}

bool HttpServer::verify_admin_auth(const HttpRequest& request) const {
    auto admin_id = request.get_header("X-Admin-Id");
    auto admin_token = request.get_header("X-Admin-Token");
    
    if (admin_id.empty() || admin_token.empty()) return false;
    return accounts_.verify_admin_password(AdminId{admin_id}, admin_token);
}

bool HttpServer::verify_class_auth(const HttpRequest& request) const {
    auto class_id = request.get_header("X-Class-Id");
    auto class_token = request.get_header("X-Class-Token");
    
    if (class_id.empty() || class_token.empty()) return false;
    return accounts_.verify_class_token(AccountId{class_id}, class_token);
}

std::string HttpServer::extract_path_pattern(const std::string& pattern, const std::string& path,
                                              const std::string& param_name) const {
    // Simple parameter extraction (could be enhanced with regex)
    std::string param_prefix = "{" + param_name + "}";
    auto pos = pattern.find(param_prefix);
    if (pos == std::string::npos) return "";
    
    std::string prefix = pattern.substr(0, pos);
    std::string suffix = pattern.substr(pos + param_prefix.size());
    
    if (path.rfind(prefix, 0) != 0) return "";
    
    auto value_start = prefix.size();
    auto value_end = path.size();
    if (!suffix.empty() && suffix.front() == '/') {
        auto slash_pos = path.find('/', value_start);
        if (slash_pos != std::string::npos) value_end = slash_pos;
    }
    
    return path.substr(value_start, value_end - value_start);
}

} // namespace turtleclass::server
