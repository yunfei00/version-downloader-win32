# Version Downloader (Win32)

当前版本：`v0.1.0`。

## 使用步骤
1. 输入 CSV URL，或点击“加载本地CSV”。
2. 选择下载目录（默认是程序目录下 `downloads`）。
3. 在列表中多选（支持 Ctrl+A）。
4. 点击“下载选中项”开始串行批量下载。
5. 下载中可点击“取消下载”。

## 版本号体系
- 应用名称：`Version Downloader`
- 当前应用版本：`0.1.0`
- Tag 版本格式：`v*`（例如 `v0.1.0`）
- 打 tag 后，GitHub Actions 会自动创建 Release，并附加 exe 和 zip。

## CSV 格式说明
```csv
name,filename,size,url
版本名,保存文件名,显示大小,下载链接
```

## 本地编译（Developer Command Prompt）
```bat
build.bat
```

执行后输出目录：
- `build/version-downloader.exe`
- `build/README.md`
- `build/sample_versions.csv`

## GitHub Actions
工作流 `.github/workflows/build-windows.yml` 支持：
1. push 到 `main` 自动构建并上传 artifact。
2. `workflow_dispatch` 手动触发构建并上传 artifact。
3. push `v*` tag 自动构建、上传 artifact、创建 Release。

## 发布版本（Tag -> Release）
```bash
git tag v0.1.0
git push origin v0.1.0
```

## 如何下载发布产物
1. 打开 GitHub 仓库的 **Releases** 页面。
2. 进入对应版本（例如 `v0.1.0`）。
3. 在 Assets 下载：
   - `version-downloader.exe`
   - `version-downloader-win32-v0.1.0.zip`

## 常见问题
- 下载失败：检查 URL 是否可访问、HTTP 状态码、目录权限、网络代理/防火墙。
- 无法编译：请确认在 Visual Studio Developer Command Prompt 中执行 `build.bat`。
