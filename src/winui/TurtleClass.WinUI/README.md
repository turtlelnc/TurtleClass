# TurtleClass.WinUI

This folder contains the Windows client scaffold for Phase 3. It is intentionally not wired into the Linux/macOS CMake build yet because WinUI 3 requires the Windows App SDK and a Windows build environment.

Current scope:

- App/window XAML placeholders for the first client shell.
- C++/WinRT entry points and page code-behind stubs.
- A thin view-model sketch that treats Core projections as display data and only exposes command intent for future sync/client layers.

Current non-goals:

- No direct state mutation from UI.
- No copied points/level/badge calculation logic.
- No formal API integration until sync protocol v1 is frozen.
- No Cloudflare or public network configuration.
