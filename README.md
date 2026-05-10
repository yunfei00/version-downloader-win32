# Version Downloader (Win32)

## 截图（占位）
![screenshot placeholder](docs/screenshot-placeholder.png)

## 功能列表
- URL 加载 CSV / 本地 CSV 文件加载
- Report ListView（全行选中、双缓冲、列宽拖动）
- 后台 WinHTTP 分块下载（可取消）
- 实时进度、速度、已下载/总大小
- 状态管理：等待下载 / 下载中 xx% / 已完成 / 已取消 / 失败
- 时间戳日志（只读自动滚动）

## CSV 示例
```csv
name,filename,size,url
v1.0.0,app-v1.0.0.zip,12.5 MB,https://example.com/a.zip
```

## 下载功能说明
- 下载目录固定为 `downloads/`，不存在时自动创建。
- 每次下载在后台线程执行，UI 线程只处理界面。
- 线程通过 `WM_APP+1/+2/+3` 回传进度、日志、完成事件。

## Win32 架构说明
- `main.cpp`：程序入口。
- `app.cpp/.h`：消息循环、窗口消息分发、业务编排。
- `ui.cpp/.h`：控件创建、布局、样式。
- `csv_parser.cpp/.h`：UTF-8 CSV 加载/解析（HTTP + 本地）。
- `download_worker.cpp/.h`：WinHTTP 下载线程。
- `logger.cpp/.h`：时间戳日志格式化。

## 本地编译（MSVC Release）
```bat
mkdir build
cl /std:c++17 /EHsc /O2 /DUNICODE /D_UNICODE /DWIN32 /D_WINDOWS /DNDEBUG ^
  src\main.cpp src\app.cpp src\ui.cpp src\csv_parser.cpp src\logger.cpp src\download_worker.cpp ^
  /Fe:build\version-downloader.exe ^
  comctl32.lib winhttp.lib comdlg32.lib shell32.lib
```
