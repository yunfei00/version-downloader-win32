# Version Downloader (Win32)

当前版本：`v0.2.0`

## V0.2.0 新功能
- 断点续传（`.part`）
- 失败自动重试（默认 3 次，1s/2s/4s）
- WinHTTP 超时设置（DNS/连接/发送/接收）
- CSV 支持可选 `sha256` 字段，且按表头名识别顺序
- 下载完成后进行大小校验；有 sha256 时进行 SHA256 校验
- `config.ini` 下载配置项增强

## 界面与编码
界面与编码说明：
- 程序界面支持简体中文显示（Unicode 宽字符串）。
- CSV 文件建议保存为 UTF-8 编码。
- 如果使用 Excel 编辑 CSV，建议“另存为 UTF-8 CSV”。

## CSV 格式
旧格式（兼容）：
```csv
name,filename,size,url
```

新格式：
```csv
name,filename,size,sha256,url
```

说明：
- 自动忽略空行
- 非法行会被忽略并写调试日志
- 有 `sha256` 时优先做 SHA256 校验；无 `sha256` 时只做大小校验

## 断点续传说明
- 下载写入 `filename.part`
- 若存在 `.part`，会尝试 Range 续传
- 服务器不支持 Range 时自动删除旧 `.part` 并重下
- 仅在校验通过后把 `.part` 重命名为正式文件

## config.ini
```ini
[General]
DownloadDir=downloads

[Download]
RetryCount=3
ConnectTimeoutMs=10000
SendTimeoutMs=30000
ReceiveTimeoutMs=30000
EnableResume=1
VerifySHA256=1
```

## 常见错误
- 服务器不支持断点续传：会自动重下
- SHA256 校验失败：保留 `.part` 供排查
- 403/404：不重试
- 下载超时/网络中断/5xx：会重试

## 本地编译
```bat
build.bat
```

## 发布
```bash
git tag v0.2.0
git push origin v0.2.0
```
