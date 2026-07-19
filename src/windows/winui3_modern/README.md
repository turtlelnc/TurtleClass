# WinUI 3 Modern Client (现代化 WinUI 3 客户端)

## 概述
这是 TurtleClass 的现代化 Windows 客户端实现，使用 WinUI 3 和 Windows App SDK。

## 技术栈
- **框架**: WinUI 3 (Windows App SDK 1.4+)
- **语言**: C++17 / WinRT
- **MVVM**: CommunityToolkit.Mvvm
- **HTTP 客户端**: WinHTTP with WinRT wrapper
- **本地存储**: SQLite (TurtleClassStorage)
- **UI 设计**: Fluent Design System

## 适用场景
- Windows 10 1809+ / Windows 11
- 现代化 UI 需求
- Fluent Design 视觉效果
- 触摸设备优化

## 构建方式
```bash
make WINDOWS_CLIENT=winui3
```

## 功能状态
- ✅ 登录/登出（现代化 UI）
- ✅ 学生列表显示（Fluent 列表控件）
- ✅ 快速加减分（动画按钮）
- ✅ 离线事件队列
- ✅ 自动同步（实时状态标签页）
- ✅ 冲突检测提示（现代化对话框）
- ✅ 撤销操作（带确认动画）
- ✅ 管理员模式（特权 UI 面板）

## 系统要求
- **操作系统**: Windows 10 1809+ 或 Windows 11
- **内存**: 最低 4GB RAM（推荐 8GB+）
- **存储**: 500MB 可用空间
- **运行时**: Windows App SDK 1.4+（安装包自带）

## 依赖项
```cmake
find_package(Microsoft.WindowsAppSDK REQUIRED)
find_package(CommunityToolkit.Mvvm REQUIRED)
```
