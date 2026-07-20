using System;
using System.Collections.Generic;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;

namespace TurtleClassWinUI;

public class SyncService
{
    private readonly AuthenticationService _authService;
    private readonly string _baseUrl;
    
    public event EventHandler<SyncState>? SyncStateChanged;
    
    public SyncState CurrentState { get; private set; } = new SyncState();

    public SyncService(AuthenticationService authService)
    {
        _authService = authService;
        _baseUrl = "https://api.turtleclass.com/api/v1";
        
        // Load pending events from local storage
        _ = LoadPendingEventsAsync();
    }

    public async Task<bool> SyncAsync()
    {
        try
        {
            CurrentState.IsSyncing = true;
            SyncStateChanged?.Invoke(this, CurrentState);

            _authService.SetAuthorizationHeader();

            // Step 1: Upload pending events
            var uploadSuccess = await UploadPendingEventsAsync();
            
            if (!uploadSuccess)
            {
                CurrentState.LastError = "Failed to upload pending events";
                CurrentState.IsSyncing = false;
                SyncStateChanged?.Invoke(this, CurrentState);
                return false;
            }

            // Step 2: Download latest events from server
            var downloadSuccess = await DownloadNewEventsAsync();
            
            if (!downloadSuccess)
            {
                CurrentState.LastError = "Failed to download new events";
                CurrentState.IsSyncing = false;
                SyncStateChanged?.Invoke(this, CurrentState);
                return false;
            }

            // Step 3: Check for conflicts
            var hasConflicts = await CheckForConflictsAsync();
            
            if (hasConflicts)
            {
                CurrentState.HasConflicts = true;
                CurrentState.LastError = "Conflicts detected - please resolve";
            }
            else
            {
                CurrentState.LastSyncTime = DateTime.UtcNow;
                CurrentState.SyncCount++;
            }

            CurrentState.IsSyncing = false;
            SyncStateChanged?.Invoke(this, CurrentState);
            
            return !hasConflicts;
        }
        catch (Exception ex)
        {
            CurrentState.IsSyncing = false;
            CurrentState.LastError = ex.Message;
            SyncStateChanged?.Invoke(this, CurrentState);
            return false;
        }
    }

    public async Task<bool> UploadPendingEventsAsync()
    {
        // Load pending events from local SQLite storage
        var pendingEvents = await LoadPendingEventsAsync();
        
        if (pendingEvents.Count == 0)
            return true; // Nothing to upload

        using var httpClient = new HttpClient();
        _authService.SetAuthorizationHeader();

        var requestUrl = $"{_baseUrl}/events/upload";
        
        var payload = new
        {
            events = pendingEvents
        };

        var json = JsonSerializer.Serialize(payload);
        var content = new StringContent(json, System.Text.Encoding.UTF8, "application/json");

        var response = await httpClient.PostAsync(requestUrl, content);
        
        if (response.IsSuccessStatusCode)
        {
            // Remove uploaded events from local storage
            await ClearUploadedEvents(pendingEvents);
            return true;
        }

        return false;
    }

    public async Task<bool> DownloadNewEventsAsync()
    {
        using var httpClient = new HttpClient();
        _authService.SetAuthorizationHeader();

        var lastSequenceId = CurrentState.LastServerSequenceId;
        var requestUrl = $"{_baseUrl}/events/download?since={lastSequenceId}";

        var response = await httpClient.GetAsync(requestUrl);
        
        if (response.IsSuccessStatusCode)
        {
            var responseJson = await response.Content.ReadAsStringAsync();
            using var doc = JsonDocument.Parse(responseJson);
            
            if (doc.RootElement.TryGetProperty("events", out var eventsElement))
            {
                // Parse and store events locally
                var newEvents = ParseEvents(eventsElement);
                await StoreEventsLocally(newEvents);
                
                // Update sequence ID
                if (doc.RootElement.TryGetProperty("latest_sequence_id", out var seqElement))
                {
                    CurrentState.LastServerSequenceId = seqElement.GetInt64();
                }
                
                return true;
            }
        }

        return false;
    }

    public async Task<bool> CheckForConflictsAsync()
    {
        using var httpClient = new HttpClient();
        _authService.SetAuthorizationHeader();

        var requestUrl = $"{_baseUrl}/conflicts/list";
        var response = await httpClient.GetAsync(requestUrl);
        
        if (response.IsSuccessStatusCode)
        {
            var responseJson = await response.Content.ReadAsStringAsync();
            using var doc = JsonDocument.Parse(responseJson);
            
            if (doc.RootElement.TryGetProperty("conflicts", out var conflictsElement))
            {
                return conflictsElement.GetArrayLength() > 0;
            }
        }

        return false;
    }

    public async Task EnqueueEventAsync(TurtleClassEvent evt)
    {
        // Sign the event with device key
        evt.DeviceSignature = await SignEventAsync(evt);
        
        // Save to local SQLite storage
        await SaveEventToLocalStorage(evt);
        
        // Trigger auto-sync if online
        if (await IsOnlineAsync())
        {
            _ = SyncAsync(); // Fire and forget
        }
    }

    private async Task<List<TurtleClassEvent>> LoadPendingEventsAsync()
    {
        // Load from TurtleClassStorage SQLite database
        // Return empty list for now
        return new List<TurtleClassEvent>();
    }

    private Task SaveEventToLocalStorage(TurtleClassEvent evt)
    {
        // Save to TurtleClassStorage SQLite database
        return Task.CompletedTask;
    }

    private Task<string> SignEventAsync(TurtleClassEvent evt)
    {
        // Sign event with Ed25519 device key
        return Task.FromResult("signature_placeholder");
    }

    private Task<bool> IsOnlineAsync()
    {
        // Check network connectivity
        return Task.FromResult(true);
    }

    private void LoadPendingEventsFromStorage()
    {
        // Load state from local storage on startup
    }
}

public class SyncState
{
    public bool IsSyncing { get; set; }
    public bool HasConflicts { get; set; }
    public DateTime LastSyncTime { get; set; }
    public int SyncCount { get; set; }
    public long LastServerSequenceId { get; set; }
    public string LastError { get; set; } = string.Empty;
}

// Placeholder for TurtleClassEvent - should reference Core library
public class TurtleClassEvent
{
    public long SequenceId { get; set; }
    public string EventType { get; set; } = string.Empty;
    public string StudentId { get; set; } = string.Empty;
    public int Points { get; set; }
    public string Reason { get; set; } = string.Empty;
    public long Timestamp { get; set; }
    public string DeviceId { get; set; } = string.Empty;
    public string DeviceSignature { get; set; } = string.Empty;
}
