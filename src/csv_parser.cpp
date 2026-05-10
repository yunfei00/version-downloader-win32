#include "csv_parser.h"

#include <windows.h>
#include <winhttp.h>

#include <sstream>
#include <vector>

#pragma comment(lib, "winhttp.lib")

namespace {
std::wstring Trim(const std::wstring& s) {
    const wchar_t* ws = L" \t\r\n";
    size_t b = s.find_first_not_of(ws);
    if (b == std::wstring::npos) return L"";
    size_t e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

bool Utf8ToWide(const std::string& in, std::wstring& out) {
    int wlen = MultiByteToWideChar(CP_UTF8, 0, in.data(), static_cast<int>(in.size()), nullptr, 0);
    if (wlen <= 0) return false;
    out.resize(wlen);
    MultiByteToWideChar(CP_UTF8, 0, in.data(), static_cast<int>(in.size()), out.data(), wlen);
    if (!out.empty() && out[0] == 0xFEFF) out.erase(out.begin());
    return true;
}
}

bool LoadUtf8TextFile(const std::wstring& path, std::wstring& outText) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD size = GetFileSize(h, nullptr);
    std::string buf(size, '\0');
    DWORD read = 0;
    bool ok = ReadFile(h, buf.data(), size, &read, nullptr);
    CloseHandle(h);
    if (!ok) return false;
    buf.resize(read);
    return Utf8ToWide(buf, outText);
}

bool LoadUtf8TextFromHttp(const std::wstring& url, std::wstring& outText, std::wstring& err) {
    URL_COMPONENTS uc{}; uc.dwStructSize = sizeof(uc);
    wchar_t host[256] = {}, path[2048] = {};
    uc.lpszHostName = host; uc.dwHostNameLength = 256;
    uc.lpszUrlPath = path; uc.dwUrlPathLength = 2048;
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &uc)) { err = L"URL解析失败"; return false; }
    HINTERNET s = WinHttpOpen(L"VersionDownloader/2.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!s) { err = L"WinHTTP会话失败"; return false; }
    HINTERNET c = WinHttpConnect(s, uc.lpszHostName, uc.nPort, 0);
    if (!c) { WinHttpCloseHandle(s); err = L"连接失败"; return false; }
    DWORD flags = uc.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET r = WinHttpOpenRequest(c, L"GET", uc.lpszUrlPath, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!r) { WinHttpCloseHandle(c); WinHttpCloseHandle(s); err = L"请求创建失败"; return false; }
    bool ok = WinHttpSendRequest(r, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(r, nullptr);
    std::string bytes;
    while (ok) {
        DWORD avail = 0; if (!WinHttpQueryDataAvailable(r, &avail)) { ok = false; break; }
        if (avail == 0) break;
        size_t old = bytes.size(); bytes.resize(old + avail);
        DWORD got = 0; if (!WinHttpReadData(r, bytes.data() + old, avail, &got)) { ok = false; break; }
        bytes.resize(old + got);
    }
    WinHttpCloseHandle(r); WinHttpCloseHandle(c); WinHttpCloseHandle(s);
    if (!ok) { err = L"HTTP读取失败"; return false; }
    return Utf8ToWide(bytes, outText);
}

bool ParseCsvText(const std::wstring& csvText, std::vector<VersionItem>& items) {
    std::wstringstream ss(csvText);
    std::wstring line;
    items.clear();
    if (!std::getline(ss, line)) return false;
    int idx = 1;
    while (std::getline(ss, line)) {
        line = Trim(line);
        if (line.empty()) continue;
        std::wstringstream ls(line);
        std::wstring cell; std::vector<std::wstring> cols;
        while (std::getline(ls, cell, L',')) cols.push_back(Trim(cell));
        if (cols.size() < 4 || cols[0].empty() || cols[1].empty() || cols[3].empty()) continue;
        VersionItem it{idx++, cols[0], cols[1], cols[2], cols[3], L"等待下载"};
        items.push_back(it);
    }
    return !items.empty();
}
