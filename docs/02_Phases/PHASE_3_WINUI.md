# Phase 3：WinUI

## 前置条件

- Phase 2 完成
- API v1 冻结
- 同步协议 v1 冻结
- CLI 模拟器通过

## 目标

实现低性能 Windows 班级电脑也能稳定运行的 WinUI 3 客户端。

## 页面

- 登录
- 首页
- 学生列表
- 学生详情
- 临时加分
- 共享模板
- 操作历史
- 同步状态
- 冲突状态
- 管理员入口
- 设置

## 架构原则

- UI 不计算积分
- UI 不计算等级
- UI 不计算徽章
- UI 不修改最终状态
- UI 只提交命令并显示 Core 投影
- 离线队列不依赖服务端在线

## 性能要求

- 控制动画
- 避免大量透明与模糊
- 列表虚拟化
- 后台同步不阻塞 UI
- 启动使用快照
- 大规模重放在后台或维护流程中完成

## 完成定义

- 离线正常加分
- 自动同步
- 重复同步不重复计分
- 撤销可用
- 模板同步可用
- 冲突提示清晰
- 管理员入口权限正确
- 在班级低性能电脑完成实机验证


## 当前进度

- 已新增 `src/winui/TurtleClass.WindowsDesktop` Windows-only native C++ desktop UI shell scaffold。
- 已包含 Win32 entry point、MainWindow、Visual Studio C++ project、resource file 与简单 ViewModel 草稿。
- 当前 UI 只展示占位学生状态并提交“命令意图”，不计算积分、等级或徽章。
- 当前未接入正式 API、同步协议、离线队列持久化或管理员入口；这些仍等待 Phase 2 API/同步协议冻结。
- 当前环境非 Windows，未执行 Windows 桌面 UI 实机构建。
