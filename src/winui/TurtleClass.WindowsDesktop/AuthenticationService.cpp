#include "AuthenticationService.hpp"

#include <sstream>
#include <codecvt>

namespace turtleclass::windows_desktop {

namespace {
    std::string wide_to_utf8(const std::wstring& wide) {
        if (wide.empty()) return "";
#ifdef _WIN32
        int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string result(size - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &result[0], size, nullptr, nullptr);
        return result;
#else
        // Simple conversion for non-Windows (may not handle all Unicode correctly)
        return std::string(wide.begin(), wide.end());
#endif
    }
    
    std::wstring utf8_to_wide(const std::string& utf8) {
        if (utf8.empty()) return L"";
#ifdef _WIN32
        int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
        std::wstring result(size - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &result[0], size);
        return result;
#else
        // Simple conversion for non-Windows
        return std::wstring(utf8.begin(), utf8.end());
#endif
    }
}

AuthenticationService::AuthenticationService() 
    : server_url_(L"https://api.turtleclass.local") {
    http_client_ = std::make_unique<HttpClient>(server_url_);
}

AuthenticationService::AuthenticationService(const std::wstring& server_url)
    : server_url_(server_url) {
    http_client_ = std::make_unique<HttpClient>(server_url_);
}

LoginResult AuthenticationService::login(const LoginCredentials& credentials) {
    if (!http_client_) {
        http_client_ = std::make_unique<HttpClient>(server_url_);
    }
    
    if (credentials.class_code.empty() || credentials.student_id.empty()) {
        return {false, L"Class code and student ID are required."};
    }
    
    // Build JSON request body
    std::ostringstream json;
    json << "{\"class_code\": \"" << wide_to_utf8(credentials.class_code) << "\",";
    json << "\"student_id\": \"" << wide_to_utf8(credentials.student_id) << "\",";
    json << "\"password\": \"" << wide_to_utf8(credentials.password) << "\"}";
    
    // Call /api/v1/auth/login endpoint
    auto response = http_client_->post(L"/api/v1/auth/login", json.str());
    
    if (response.status_code == 200) {
        // Parse response (simplified - use proper JSON parser in production)
        // Expected: {"device_token": "...", "server_url": "..."}
        logged_in_ = true;
        
        // Extract device_token from response (simplified parsing)
        std::string body = response.body;
        auto token_pos = body.find("\"device_token\"");
        if (token_pos != std::string::npos) {
            auto start = body.find('"', token_pos + 14);
            if (start != std::string::npos) {
                auto end = body.find('"', start + 1);
                if (end != std::string::npos) {
                    device_token_ = utf8_to_wide(body.substr(start + 1, end - start - 1));
                }
            }
        }
        
        if (device_token_.empty()) {
            device_token_ = L"unknown_token";
        }
        
        http_client_->set_auth_token(device_token_);
        
        if (login_state_changed_callback_) {
            login_state_changed_callback_(true);
        }
        
        return {true, L"Login successful.", device_token_, server_url_};
    } else if (response.status_code == 401) {
        return {false, L"Invalid credentials. Please check your class code, student ID, and password."};
    } else if (response.status_code == 0) {
        return {false, L"Network error: " + response.error_message};
    } else {
        return {false, L"Login failed with status: " + std::to_wstring(response.status_code)};
    }
}

bool AuthenticationService::is_logged_in() const noexcept {
    return logged_in_;
}

const std::wstring& AuthenticationService::get_device_token() const noexcept {
    return device_token_;
}

const std::wstring& AuthenticationService::get_server_url() const noexcept {
    return server_url_;
}

void AuthenticationService::logout() {
    logged_in_ = false;
    device_token_.clear();
    
    if (http_client_) {
        http_client_->clear_auth_token();
    }
    
    if (login_state_changed_callback_) {
        login_state_changed_callback_(false);
    }
}

void AuthenticationService::set_login_state_changed_callback(LoginStateChangedCallback callback) {
    login_state_changed_callback_ = std::move(callback);
}

} // namespace turtleclass::windows_desktop
