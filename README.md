# Version Downloader (Win32)

## 项目目标
这是一个超轻量 Windows 原生 C++ 版本下载工具：从 CSV 版本列表加载数据，展示在表格中，并下载选中版本文件。

## 为什么选择 Win32 API
- 无需 Qt/.NET/Python 运行时
- 可编译为单个 EXE
- 直接使用系统自带控件与网络能力（WinHTTP/URLMon）
- 兼容 Windows 10/11

## 本地编译（MSVC）
在 “x64 Native Tools Command Prompt for VS” 中执行：

```bat
mkdir build
cl /std:c++17 /EHsc /O2 /DUNICODE /D_UNICODE /DWIN32 /D_WINDOWS /DNDEBUG ^
  src\main.cpp src\app.cpp src\downloader.cpp ^
  /Fe:build\version-downloader.exe ^
  comctl32.lib urlmon.lib shlwapi.lib winhttp.lib
```

## GitHub Actions 生成 EXE
- Workflow: `.github/workflows/build-windows.yml`
- 触发方式：
  - push 到 `main`
  - 手动触发 `workflow_dispatch`
- 产物：`build/version-downloader.exe`
- 通过 Artifact 下载 `version-downloader`

## CSV 示例格式
第一行必须是表头：

```csv
name,filename,size,url
v1.0.0,app-v1.0.0.zip,12.5 MB,https://example.com/app-v1.0.0.zip
v1.1.0,app-v1.1.0.zip,13.2 MB,https://example.com/app-v1.1.0.zip
```

## 当前功能列表
- 主窗口标题：`Version Downloader`
- 固定初始窗口大小：`900 x 600`
- 顶部 URL 输入 + 加载按钮
- 中间 ListView 列：序号/版本名称/文件名/大小/下载地址/状态
- CSV 解析并填充表格（序号从 1 开始，size 原样显示）
- 下载选中项（保存对话框默认文件名来自 `filename`）
- 后台线程下载，UI 不阻塞
- 进度条实时更新
- 状态列更新：等待下载 / 下载中 xx% / 已完成 / 已取消 / 失败
- 日志框追加：加载、下载进度、完成、失败、取消
- 取消下载
