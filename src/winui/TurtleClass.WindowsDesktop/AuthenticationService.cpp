#include "AuthenticationService.hpp"

namespace turtleclass::windows_desktop {

LoginResult AuthenticationService::login(const LoginCredentials& credentials) {
    // TODO: Implement actual HTTP API call to /api/v1/auth/login
    // For now, return a placeholder result
    
    if (credentials.class_code.empty() || credentials.student_id.empty()) {
        return {false, L"Class code and student ID are required."};
    }
    
    // Placeholder: In production, this would call the server API
    // and receive a JWT device token upon successful authentication
    logged_in_ = true;
    device_token_ = L"placeholder_jwt_token_" + credentials.student_id;
    server_url_ = L"https://api.turtleclass.example.com";
    
    if (login_state_changed_callback_) {
        login_state_changed_callback_(true);
    }
    
    return {true, L"Login successful. Device registered.", device_token_, server_url_};
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
    server_url_.clear();
    
    if (login_state_changed_callback_) {
        login_state_changed_callback_(false);
    }
}

void AuthenticationService::set_login_state_changed_callback(LoginStateChangedCallback callback) {
    login_state_changed_callback_ = std::move(callback);
}

} // namespace turtleclass::windows_desktop
