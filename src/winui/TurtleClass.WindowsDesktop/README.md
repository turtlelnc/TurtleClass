# TurtleClass.WindowsDesktop

This folder contains the Windows client UI as a native C++ desktop application using WinUI 3.

## Architecture Decisions

- **Framework**: WinUI 3 with C++/WinRT for modern Windows UI experience
- **Pattern**: MVVM (Model-View-ViewModel) for separation of concerns
- **Language**: Native C++ for performance and Core integration
- **Authentication**: JWT-based device authentication via server API
- **Sync Model**: Offline-first with local event queue and automatic sync

## Features Implemented (Phase 3)

### Authentication
- Login with class code, student ID, and password
- Device registration and JWT token storage
- Logout functionality
- UI state management based on login status

### Student Management
- Student list display with level, points, and badges
- Quick points adjustment interface
- Offline event queuing for later sync

### UI Components
- `MainWindow`: Main application window with login and student views
- `AuthenticationService`: Handles authentication logic and token management
- `StudentListViewModel`: Projects Core state for UI display

## Build Requirements

- Windows 10 SDK (10.0.19041.0 or later)
- Visual Studio 2022 with C++ desktop development workload
- WinUI 3 project templates
- MSVC v143 or later toolset

## Building

1. Open `TurtleClass.WindowsDesktop.vcxproj` in Visual Studio
2. Select configuration (Debug/Release) and platform (x64)
3. Build solution (Ctrl+Shift+B)
4. Run the executable from the output directory

## Current Status

- ✅ Login/logout flow
- ✅ Student list display
- ✅ Quick points queue (offline)
- ✅ UI state management
- 🔄 Server API integration (placeholder)
- 🔄 Real sync protocol implementation
- 🔄 Local event queue persistence
- 🔄 Conflict detection UI
- 🔄 Admin mode entry

## Non-Goals (for now)

- No direct state mutation from UI (all through Core)
- No point/level/badge calculations in UI layer
- No Cloudflare or public network configuration yet

## Next Steps

1. Integrate with frozen API v1 endpoints
2. Implement local SQLite storage for offline queue
3. Add sync protocol v1 client
4. Implement conflict resolution UI
5. Add admin mode toggle and features
