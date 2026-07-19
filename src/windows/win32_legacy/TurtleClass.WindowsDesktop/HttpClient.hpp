#pragma once

#include <string>
#include <map>
#include <optional>

namespace turtleclass::windows_desktop {

struct HttpResponse {
    int status_code = 200;
    std::map<std::string, std::string> headers;
    std::string body;
    std::wstring error_message;
};

class HttpClient {
public:
    explicit HttpClient(const std::wstring& base_url);
    
    HttpResponse get(const std::wstring& path);
    HttpResponse post(const std::wstring& path, const std::string& body, 
                     const std::map<std::string, std::string>& headers = {});
    
    void set_auth_token(const std::wstring& token);
    void clear_auth_token();
    
private:
    std::wstring base_url_;
    std::wstring auth_token_;
    
    HttpResponse send_request(const std::wstring& method, const std::wstring& path,
                             const std::string& body = "", 
                             const std::map<std::string, std::string>& extra_headers = {});
};

} // namespace turtleclass::windows_desktop
