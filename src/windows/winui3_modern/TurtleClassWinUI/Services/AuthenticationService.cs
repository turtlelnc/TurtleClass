using System;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using Windows.Security.Cryptography;
using Windows.Storage.Streams;

namespace TurtleClassWinUI;

public class AuthenticationService
{
    private readonly HttpClient _httpClient;
    private readonly string _baseUrl;
    
    private string? _studentToken;
    private string? _adminToken;
    private string? _deviceToken;
    private string? _deviceId;

    public event EventHandler<bool>? LoginStateChanged;

    public bool IsLoggedIn => !string.IsNullOrEmpty(_studentToken);
    public bool IsAdmin => !string.IsNullOrEmpty(_adminToken);

    public AuthenticationService()
    {
        _httpClient = new HttpClient();
        _baseUrl = "https://api.turtleclass.com/api/v1"; // Production URL
        
        // Load saved tokens from local storage (implement in TurtleClassStorage)
        LoadTokens();
    }

    public async Task<bool> LoginAsync(string classCode, string studentId, string password)
    {
        try
        {
            var requestUrl = $"{_baseUrl}/auth/login";
            
            var payload = new
            {
                class_code = classCode,
                student_id = studentId,
                password = password,
                device_id = await GetOrCreateDeviceIdAsync()
            };

            var json = JsonSerializer.Serialize(payload);
            var content = new StringContent(json, Encoding.UTF8, "application/json");

            var response = await _httpClient.PostAsync(requestUrl, content);
            
            if (response.IsSuccessStatusCode)
            {
                var responseJson = await response.Content.ReadAsStringAsync();
                using var doc = JsonDocument.Parse(responseJson);
                
                if (doc.RootElement.TryGetProperty("token", out var tokenElement))
                {
                    _studentToken = tokenElement.GetString();
                    
                    // Save tokens to local storage
                    SaveTokens();
                    
                    LoginStateChanged?.Invoke(this, true);
                    return true;
                }
            }
            
            return false;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Login error: {ex.Message}");
            return false;
        }
    }

    public async Task<bool> AdminLoginAsync(string adminUsername, string adminPassword)
    {
        try
        {
            var requestUrl = $"{_baseUrl}/auth/admin/login";
            
            var payload = new
            {
                username = adminUsername,
                password = adminPassword,
                device_id = await GetOrCreateDeviceIdAsync()
            };

            var json = JsonSerializer.Serialize(payload);
            var content = new StringContent(json, Encoding.UTF8, "application/json");

            var response = await _httpClient.PostAsync(requestUrl, content);
            
            if (response.IsSuccessStatusCode)
            {
                var responseJson = await response.Content.ReadAsStringAsync();
                using var doc = JsonDocument.Parse(responseJson);
                
                if (doc.RootElement.TryGetProperty("token", out var tokenElement))
                {
                    _adminToken = tokenElement.GetString();
                    SaveTokens();
                    LoginStateChanged?.Invoke(this, true);
                    return true;
                }
            }
            
            return false;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Admin login error: {ex.Message}");
            return false;
        }
    }

    public void Logout()
    {
        _studentToken = null;
        _adminToken = null;
        ClearTokens();
        LoginStateChanged?.Invoke(this, false);
    }

    public void SetAuthorizationHeader()
    {
        if (!string.IsNullOrEmpty(_studentToken))
        {
            _httpClient.DefaultRequestHeaders.Authorization = 
                new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", _studentToken);
        }
        else if (!string.IsNullOrEmpty(_adminToken))
        {
            _httpClient.DefaultRequestHeaders.Authorization = 
                new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", _adminToken);
        }
    }

    private async Task<string> GetOrCreateDeviceIdAsync()
    {
        if (!string.IsNullOrEmpty(_deviceId))
            return _deviceId;

        // Try to load from local storage
        _deviceId = await LoadDeviceIdAsync();
        
        if (string.IsNullOrEmpty(_deviceId))
        {
            // Generate new device ID
            _deviceId = Guid.NewGuid().ToString("N");
            await SaveDeviceIdAsync(_deviceId);
        }

        return _deviceId;
    }

    private void SaveTokens()
    {
        // Implement in TurtleClassStorage - save to SQLite with encryption
        // For now, use ApplicationDataContainer as placeholder
        var localSettings = Windows.Storage.ApplicationData.Current.LocalSettings;
        localSettings.Values["StudentToken"] = _studentToken;
        localSettings.Values["AdminToken"] = _adminToken;
        localSettings.Values["DeviceId"] = _deviceId;
    }

    private void LoadTokens()
    {
        var localSettings = Windows.Storage.ApplicationData.Current.LocalSettings;
        
        if (localSettings.Values.TryGetValue("StudentToken", out var studentToken))
            _studentToken = studentToken as string;
        
        if (localSettings.Values.TryGetValue("AdminToken", out var adminToken))
            _adminToken = adminToken as string;
        
        if (localSettings.Values.TryGetValue("DeviceId", out var deviceId))
            _deviceId = deviceId as string;
    }

    private void ClearTokens()
    {
        var localSettings = Windows.Storage.ApplicationData.Current.LocalSettings;
        localSettings.Values.Remove("StudentToken");
        localSettings.Values.Remove("AdminToken");
    }

    private Task<string> LoadDeviceIdAsync()
    {
        var localSettings = Windows.Storage.ApplicationData.Current.LocalSettings;
        return Task.FromResult(localSettings.Values.TryGetValue("DeviceId", out var deviceId) 
            ? (deviceId as string) ?? string.Empty 
            : string.Empty);
    }

    private Task SaveDeviceIdAsync(string deviceId)
    {
        var localSettings = Windows.Storage.ApplicationData.Current.LocalSettings;
        localSettings.Values["DeviceId"] = deviceId;
        return Task.CompletedTask;
    }
}
