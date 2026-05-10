# Version Downloader (Win32)

## 使用步骤
1. 输入 CSV URL，或点击“加载本地CSV”。
2. 选择下载目录（默认是程序目录下 `downloads`）。
3. 在列表中多选（支持 Ctrl+A）。
4. 点击“下载选中项”开始串行批量下载。
5. 下载中可点击“取消下载”。

## CSV 格式说明
```csv
name,filename,size,url
版本名,保存文件名,显示大小,下载链接
```
- `size` 仅用于展示；实际下载进度和校验以 HTTP `Content-Length` 为准。
- 若服务器不返回 `Content-Length`，界面显示“未知大小”，仍可下载。

## 批量下载说明
- 支持 ListView 多选后串行下载（不并发）。
- 状态列会显示：等待下载 / 排队中 / 下载中 xx% / 已完成 / 已取消 / 失败。
- 日志会记录总数、当前序号、每个文件结果和最终统计。

## 下载目录说明
- 默认目录：`<exe目录>\downloads`
- 可在界面中修改并保存到 `config.ini`：
```ini
[General]
DownloadDir=D:\downloads
```

## GitHub Actions 产物
工作流会上传：
- `version-downloader.exe`
- `version-downloader-win32.zip`（包含 exe、README.md、sample_versions.csv）

## 从 Actions 下载 EXE
1. 打开仓库 Actions。
2. 进入 `Build Windows EXE`。
3. 在 Artifacts 下载 `version-downloader.exe` 或 `version-downloader-win32.zip`。

## 常见问题
- 为什么不用 Qt：为保持零额外运行时依赖，使用纯 Win32 API + Common Controls。
- 为什么 exe 很小：未引入大型 GUI 框架，仅链接系统库。
- 下载失败如何排查：检查 URL 是否可访问、HTTP 状态码、目录权限、网络代理/防火墙。

## 本地编译（MSVC Release）
```bat
mkdir build
cl /std:c++17 /EHsc /O2 /DUNICODE /D_UNICODE /DWIN32 /D_WINDOWS /DNDEBUG ^
  src\main.cpp src\app.cpp src\ui.cpp src\csv_parser.cpp src\logger.cpp src\download_worker.cpp ^
  /Fe:build\version-downloader.exe ^
  comctl32.lib winhttp.lib comdlg32.lib shell32.lib
```
