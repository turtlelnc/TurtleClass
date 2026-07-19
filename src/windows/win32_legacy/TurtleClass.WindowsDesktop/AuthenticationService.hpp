#pragma once

#include "HttpClient.hpp"

#include <string>
#include <optional>
#include <functional>
#include <memory>

namespace turtleclass::windows_desktop {

struct LoginCredentials {
    std::wstring class_code;
    std::wstring student_id;
    std::wstring password;
};

struct LoginResult {
    bool success = false;
    std::wstring message;
    std::wstring device_token;  // JWT or similar
    std::wstring server_url;
};

class AuthenticationService {
public:
    AuthenticationService();
    explicit AuthenticationService(const std::wstring& server_url);
    
    [[nodiscard]] LoginResult login(const LoginCredentials& credentials);
    [[nodiscard]] bool is_logged_in() const noexcept;
    [[nodiscard]] const std::wstring& get_device_token() const noexcept;
    [[nodiscard]] const std::wstring& get_server_url() const noexcept;
    void logout();
    
    // Signal for login state changes
    using LoginStateChangedCallback = std::function<void(bool logged_in)>;
    void set_login_state_changed_callback(LoginStateChangedCallback callback);

private:
    std::wstring device_token_;
    std::wstring server_url_;
    bool logged_in_ = false;
    LoginStateChangedCallback login_state_changed_callback_;
    std::unique_ptr<HttpClient> http_client_;
};

} // namespace turtleclass::windows_desktop
