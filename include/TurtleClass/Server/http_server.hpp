#pragma once

#include "TurtleClass/Server/server.hpp"
#include "TurtleClass/Server/accounts.hpp"
#include "TurtleClass/Server/snapshot.hpp"

#include <string>
#include <map>
#include <functional>
#include <cstdint>

namespace turtleclass::server {

struct HttpRequest {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
    
    std::string get_header(const std::string& name) const {
        auto it = headers.find(name);
        return (it != headers.end()) ? it->second : "";
    }
    
    std::string query_param(const std::string& name) const {
        auto pos = path.find('?');
        if (pos == std::string::npos) return "";
        
        std::string query = path.substr(pos + 1);
        auto start = query.find(name + "=");
        if (start == std::string::npos) return "";
        
        start += name.size() + 1;
        auto end = query.find('&', start);
        if (end == std::string::npos) end = query.size();
        
        return query.substr(start, end - start);
    }
};

struct HttpResponse {
    int status_code = 200;
    std::map<std::string, std::string> headers;
    std::string body;
    
    void set_json_content() {
        headers["Content-Type"] = "application/json";
    }
    
    static HttpResponse ok(const std::string& body) {
        HttpResponse resp;
        resp.status_code = 200;
        resp.body = body;
        resp.set_json_content();
        return resp;
    }
    
    static HttpResponse bad_request(const std::string& message) {
        HttpResponse resp;
        resp.status_code = 400;
        resp.body = "{\"error\": \"" + message + "\"}";
        resp.set_json_content();
        return resp;
    }
    
    static HttpResponse unauthorized(const std::string& message) {
        HttpResponse resp;
        resp.status_code = 401;
        resp.body = "{\"error\": \"" + message + "\"}";
        resp.set_json_content();
        return resp;
    }
    
    static HttpResponse forbidden(const std::string& message) {
        HttpResponse resp;
        resp.status_code = 403;
        resp.body = "{\"error\": \"" + message + "\"}";
        resp.set_json_content();
        return resp;
    }
    
    static HttpResponse not_found(const std::string& message) {
        HttpResponse resp;
        resp.status_code = 404;
        resp.body = "{\"error\": \"" + message + "\"}";
        resp.set_json_content();
        return resp;
    }
    
    static HttpResponse internal_error(const std::string& message) {
        HttpResponse resp;
        resp.status_code = 500;
        resp.body = "{\"error\": \"" + message + "\"}";
        resp.set_json_content();
        return resp;
    }
};

using HttpHandler = std::function<HttpResponse(const HttpRequest&)>;

class HttpServer {
public:
    explicit HttpServer(ServerBackend& backend, AccountStore& accounts, 
                       DeviceTable& devices, SnapshotManager& snapshot_mgr);
    
    void register_route(const std::string& method, const std::string& path, HttpHandler handler);
    HttpResponse handle_request(const HttpRequest& request);
    
    // Built-in handlers
    HttpResponse handle_health(const HttpRequest& request);
    HttpResponse handle_login(const HttpRequest& request);
    HttpResponse handle_register_device(const HttpRequest& request);
    HttpResponse handle_revoke_device(const HttpRequest& request);
    HttpResponse handle_upload_events(const HttpRequest& request);
    HttpResponse handle_download_events(const HttpRequest& request);
    HttpResponse handle_get_snapshot(const HttpRequest& request);
    HttpResponse handle_list_conflicts(const HttpRequest& request);
    HttpResponse handle_resolve_conflict(const HttpRequest& request);
    HttpResponse handle_export_events(const HttpRequest& request);
    HttpResponse handle_export_logs(const HttpRequest& request);
    
private:
    ServerBackend& backend_;
    AccountStore& accounts_;
    DeviceTable& devices_;
    SnapshotManager& snapshot_mgr_;
    
    std::map<std::string, HttpHandler> routes_;
    
    bool verify_admin_auth(const HttpRequest& request) const;
    bool verify_class_auth(const HttpRequest& request) const;
    std::string extract_path_pattern(const std::string& pattern, const std::string& path, 
                                     const std::string& param_name) const;
};

} // namespace turtleclass::server
