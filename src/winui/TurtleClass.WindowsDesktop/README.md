# TurtleClass.WindowsDesktop

This folder contains the Windows client UI scaffold as a native C++ desktop application.

Important decisions:

- This is a C++ Windows desktop UI scaffold, not a C# WinUI project.
- The first UI implementation uses the Win32 desktop application model so it can be extended from C++ without introducing C#.
- It is not wired into the cross-platform CMake build because it requires a Windows SDK and MSVC/Visual Studio on Windows.
- UI code must not calculate points, levels, or badges. It displays state projected by Core and queues command intent for future sync/client layers.

Current non-goals:

- No formal API integration until sync protocol v1 is frozen.
- No direct state mutation from UI.
- No Cloudflare or public network configuration.
