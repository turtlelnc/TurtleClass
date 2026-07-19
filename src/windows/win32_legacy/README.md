# Win32 Legacy Client (传统 Win32 C++ 客户端)

## 概述
这是 TurtleClass 的传统 Windows 客户端实现，使用原生 Win32 C++ API。

## 技术栈
- **框架**: 原生 Win32 API
- **语言**: C++17
- **HTTP 客户端**: WinHTTP
- **本地存储**: SQLite (TurtleClassStorage)

## 适用场景
- 低性能设备（<4GB RAM）
- Windows 7/8.1 兼容性需求
- 最小化依赖部署
- 快速启动需求（<2 秒）

## 构建方式
```bash
make WINDOWS_CLIENT=win32
```

## 功能状态
- ✅ 登录/登出
- ✅ 学生列表显示
- ✅ 快速加减分
- ✅ 离线事件队列
- ✅ 自动同步
- ✅ 冲突检测提示
- ✅ 撤销操作
- ✅ 管理员模式

## 限制
- UI 较为简单（非现代化界面）
- 不支持 Fluent Design
- 无动画效果
