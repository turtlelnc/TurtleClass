# Windows 客户端构建说明

TurtleClass 现在支持两种 Windows 客户端实现：

## 🖥️ Win32 Legacy Client（传统客户端）

**适用场景：**
- 低性能设备（<4GB RAM）
- Windows 7/8.1 兼容性需求
- 最小化依赖部署
- 快速启动需求（<2 秒）

**技术栈：**
- 原生 Win32 C++ API
- C++17
- WinHTTP
- SQLite

**构建方式：**
```bash
# Linux/macOS (交叉编译准备)
cmake -B build -DWINDOWS_CLIENT=win32
cmake --build build

# Windows (MSVC)
cmake -B build -G "Visual Studio 17 2022" -DWINDOWS_CLIENT=win32
cmake --build build --config Release
```

**输出：** `build/TurtleClassClient.exe`

---

## ✨ WinUI 3 Modern Client（现代化客户端）

**适用场景：**
- Windows 10 1809+ / Windows 11
- 现代化 UI 需求
- Fluent Design 视觉效果
- 触摸设备优化

**技术栈：**
- WinUI 3 (Windows App SDK 1.4+)
- C++17 / WinRT
- CommunityToolkit.Mvvm
- WinHTTP with WinRT wrapper
- SQLite

**系统要求：**
- Windows 10 1809+ 或 Windows 11
- 最低 4GB RAM（推荐 8GB+）
- 500MB 可用空间

**构建方式：**
```bash
# Windows (MSVC) - 需要 Windows App SDK
cmake -B build -G "Visual Studio 17 2022" -DWINDOWS_CLIENT=winui3
cmake --build build --config Release
```

**输出：** `build/Release/TurtleClassClient.exe`

---

## 🔧 Makefile 快捷方式

```bash
# 构建 Win32 版本
make WINDOWS_CLIENT=win32

# 构建 WinUI 3 版本
make WINDOWS_CLIENT=winui3

# 默认构建（Win32）
make
```

---

## 📊 功能对比

| 功能 | Win32 Legacy | WinUI 3 Modern |
|------|-------------|----------------|
| 登录/登出 | ✅ | ✅ |
| 学生列表 | ✅ | ✅ (Fluent 控件) |
| 快速加减分 | ✅ | ✅ (动画按钮) |
| 离线队列 | ✅ | ✅ |
| 自动同步 | ✅ | ✅ (实时状态) |
| 冲突提示 | ✅ | ✅ (现代化对话框) |
| 撤销操作 | ✅ | ✅ (带动画) |
| 管理员模式 | ✅ | ✅ (特权面板) |
| Fluent Design | ❌ | ✅ |
| 动画效果 | ❌ | ✅ |
| 触摸优化 | ⚠️ 基础 | ✅ 完整 |
| 内存占用 | ~30MB | ~80MB |
| 启动时间 | <2 秒 | ~3 秒 |
| Windows 7/8.1 | ✅ | ❌ |

---

## 🎯 选择建议

**选择 Win32 Legacy，如果：**
- 需要在旧电脑上运行
- 内存资源有限
- 追求最快启动速度
- 不需要现代化 UI

**选择 WinUI 3 Modern，如果：**
- 目标用户主要是 Windows 10/11
- 需要现代化外观
- 需要触摸设备支持
- 希望有流畅的动画效果

---

## 📦 安装包生成

### Win32 MSI 安装包
```bash
cd deploy
./build_installer.sh win32
# 输出：TurtleClass-v2.1.0-win32.msi
```

### WinUI 3 MSIX 安装包
```bash
cd deploy
./build_installer.sh winui3
# 输出：TurtleClass-v2.1.0-winui3.msix
```
