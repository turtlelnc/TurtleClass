#include "HttpClient.hpp"

#ifdef _WIN32
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

#include <sstream>
#include <codecvt>

namespace turtleclass::windows_desktop {

HttpClient::HttpClient(const std::wstring& base_url) 
    : base_url_(base_url) {
}

void HttpClient::set_auth_token(const std::wstring& token) {
    auth_token_ = token;
}

void HttpClient::clear_auth_token() {
    auth_token_.clear();
}

HttpResponse HttpClient::get(const std::wstring& path) {
    return send_request(L"GET", path);
}

HttpResponse HttpClient::post(const std::wstring& path, const std::string& body,
                              const std::map<std::string, std::string>& headers) {
    std::map<std::string, std::string> all_headers = headers;
    if (!all_headers.count("Content-Type")) {
        all_headers["Content-Type"] = "application/json";
    }
    return send_request(L"POST", path, body, all_headers);
}

HttpResponse HttpClient::send_request(const std::wstring& method, const std::wstring& path,
                                      const std::string& body,
                                      const std::map<std::string, std::string>& extra_headers) {
    HttpResponse response;
    
#ifdef _WIN32
    try {
        // Parse URL
        URL_COMPONENTS url_comp = {};
        url_comp.dwStructSize = sizeof(url_comp);
        url_comp.dwSchemeLength = -1;
        url_comp.dwHostNameLength = -1;
        url_comp.dwUrlPathLength = -1;
        
        std::wstring full_url = base_url_ + path;
        
        if (!WinHttpCrackUrl(full_url.c_str(), 0, 0, &url_comp)) {
            response.status_code = 0;
            response.error_message = L"Failed to parse URL";
            return response;
        }
        
        // Create session
        HINTERNET hSession = WinHttpOpen(L"TurtleClass/1.0",
                                         WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                         WINHTTP_NO_PROXY_NAME,
                                         WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) {
            response.status_code = 0;
            response.error_message = L"Failed to create HTTP session";
            return response;
        }
        
        // Connect to server
        std::wstring host(url_comp.lpszHostName, url_comp.dwHostNameLength);
        HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(),
                                            url_comp.nPort, 0);
        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            response.status_code = 0;
            response.error_message = L"Failed to connect to server";
            return response;
        }
        
        // Create request
        std::wstring url_path(url_comp.lpszUrlPath, url_comp.dwUrlPathLength);
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, method.c_str(),
                                                 url_path.c_str(), nullptr,
                                                 WINHTTP_NO_REFERER,
                                                 WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                 (url_comp.nScheme == INTERNET_SCHEME_HTTPS) ? 
                                                     WINHTTP_FLAG_SECURE : 0);
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            response.status_code = 0;
            response.error_message = L"Failed to create HTTP request";
            return response;
        }
        
        // Add headers
        for (const auto& [header_name, header_value] : extra_headers) {
            std::string header_line = header_name + ": " + header_value;
            std::wstring wide_header(header_line.begin(), header_line.end());
            WinHttpAddRequestHeaders(hRequest, wide_header.c_str(), -1, 
                                    WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
        }
        
        // Add auth header if present
        if (!auth_token_.empty()) {
            std::wstring auth_header = L"Authorization: Bearer " + auth_token_;
            WinHttpAddRequestHeaders(hRequest, auth_header.c_str(), -1,
                                    WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
        }
        
        // Send request
        BOOL bResult = WinHttpSendRequest(hRequest,
                                          WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                          const_cast<char*>(body.data()),
                                          static_cast<DWORD>(body.size()),
                                          static_cast<DWORD>(body.size()), 0);
        
        if (!bResult || !WinHttpReceiveResponse(hRequest)) {
            DWORD error = GetLastError();
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            response.status_code = 0;
            response.error_message = L"Request failed: " + std::to_wstring(error);
            return response;
        }
        
        // Get status code
        DWORD status_code = 0;
        DWORD size = sizeof(status_code);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                           WINHTTP_HEADER_NAME_BY_INDEX, &status_code, &size, 
                           WINHTTP_NO_HEADER_INDEX);
        response.status_code = static_cast<int>(status_code);
        
        // Read response body
        std::string response_body;
        char buffer[4096];
        DWORD bytes_read = 0;
        
        do {
            bytes_read = 0;
            if (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytes_read)) {
                response_body.append(buffer, bytes_read);
            }
        } while (bytes_read > 0);
        
        response.body = response_body;
        
        // Clean up
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        
    } catch (const std::exception& e) {
        response.status_code = 0;
        std::string msg = e.what();
        response.error_message = std::wstring(msg.begin(), msg.end());
    } catch (...) {
        response.status_code = 0;
        response.error_message = L"Unknown error occurred";
    }
#else
    // Non-Windows placeholder - for testing on Linux
    response.status_code = 200;
    response.body = "{\"status\": \"placeholder\"}";
#endif
    
    return response;
}

} // namespace turtleclass::windows_desktop
