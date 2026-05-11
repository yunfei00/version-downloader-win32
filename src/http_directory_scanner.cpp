#include "pch.h"
#include "http_directory_scanner.h"
#include "string_utils.h"
#include <winhttp.h>
#include <set>

#pragma comment(lib, "winhttp.lib")

namespace {
bool IsIgnoredHref(const std::wstring& href) {
    if (href.empty()) return true;
    if (href == L"../" || href == L"/" || href == L"?C=N;O=D" || href == L"?C=M;O=A") return true;
    if (href[0] == L'#') return true;
    return false;
}

bool IsHttpLike(const std::wstring& s) {
    return s.rfind(L"http://", 0) == 0 || s.rfind(L"https://", 0) == 0;
}

std::wstring JoinUrl(const std::wstring& base, const std::wstring& href) {
    if (IsHttpLike(href)) return href;
    if (href.rfind(L"//", 0) == 0) {
        return (base.rfind(L"https://", 0) == 0 ? L"https:" : L"http:") + href;
    }
    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);
    wchar_t host[512]{};
    wchar_t path[2048]{};
    uc.lpszHostName = host; uc.dwHostNameLength = 512;
    uc.lpszUrlPath = path; uc.dwUrlPathLength = 2048;
    if (!WinHttpCrackUrl(base.c_str(), 0, 0, &uc)) return href;
    std::wstring root = std::wstring(uc.nScheme == INTERNET_SCHEME_HTTPS ? L"https://" : L"http://") + std::wstring(uc.lpszHostName, uc.dwHostNameLength);
    if ((uc.nScheme == INTERNET_SCHEME_HTTP && uc.nPort != INTERNET_DEFAULT_HTTP_PORT) || (uc.nScheme == INTERNET_SCHEME_HTTPS && uc.nPort != INTERNET_DEFAULT_HTTPS_PORT)) {
        root += L":" + std::to_wstring(uc.nPort);
    }
    if (!href.empty() && href[0] == L'/') return root + href;
    std::wstring basePath(uc.lpszUrlPath, uc.dwUrlPathLength);
    auto slash = basePath.find_last_of(L'/');
    std::wstring dir = (slash == std::wstring::npos) ? L"/" : basePath.substr(0, slash + 1);
    return root + dir + href;
}

std::vector<std::wstring> ExtractHrefs(const std::wstring& html) {
    std::vector<std::wstring> out;
    size_t pos = 0;
    while (true) {
        size_t a = html.find(L"href", pos);
        if (a == std::wstring::npos) break;
        size_t eq = html.find(L'=', a);
        if (eq == std::wstring::npos) break;
        size_t s = html.find_first_not_of(L" \t\r\n", eq + 1);
        if (s == std::wstring::npos) break;
        wchar_t quote = 0;
        if (html[s] == L'\'' || html[s] == L'\"') { quote = html[s]; ++s; }
        size_t e = quote ? html.find(quote, s) : html.find_first_of(L" \t\r\n>", s);
        if (e == std::wstring::npos) break;
        out.push_back(html.substr(s, e - s));
        pos = e + 1;
    }
    return out;
}

void FetchSizeByHead(RemoteFileItem& item) {
    URL_COMPONENTS uc{}; uc.dwStructSize = sizeof(uc);
    wchar_t host[512]{}, path[4096]{};
    uc.lpszHostName = host; uc.dwHostNameLength = 512;
    uc.lpszUrlPath = path; uc.dwUrlPathLength = 4096;
    if (!WinHttpCrackUrl(item.url.c_str(), 0, 0, &uc)) return;
    HINTERNET s = WinHttpOpen(L"HttpPathDownloader/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!s) return;
    HINTERNET c = WinHttpConnect(s, uc.lpszHostName, uc.nPort, 0);
    DWORD f = uc.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET r = c ? WinHttpOpenRequest(c, L"HEAD", uc.lpszUrlPath, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, f) : nullptr;
    if (r && WinHttpSendRequest(r, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(r, nullptr)) {
        unsigned long long len = 0; DWORD n = sizeof(len);
        if (WinHttpQueryHeaders(r, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &len, &n, WINHTTP_NO_HEADER_INDEX)) {
            item.size = len; item.sizeKnown = true;
        }
    }
    if (r) WinHttpCloseHandle(r); if (c) WinHttpCloseHandle(c); WinHttpCloseHandle(s);
}
}

bool ScanHttpDirectory(const std::wstring& directoryUrl, std::vector<RemoteFileItem>& outFiles, std::wstring& error) {
    outFiles.clear();
    std::wstring html;
    if (!LoadUtf8TextFromHttp(directoryUrl, html, error)) return false;
    auto hrefs = ExtractHrefs(html);
    std::set<std::wstring> seen;
    for (const auto& h : hrefs) {
        if (IsIgnoredHref(h)) continue;
        auto full = JoinUrl(directoryUrl, h);
        if (full.empty() || !seen.insert(full).second) continue;
        if (!h.empty() && h.back() == L'/') continue;
        RemoteFileItem it;
        it.url = full;
        auto p = full.find_last_of(L'/');
        it.name = (p == std::wstring::npos) ? full : full.substr(p + 1);
        if (it.name.empty()) continue;
        FetchSizeByHead(it);
        outFiles.push_back(std::move(it));
    }
    return true;
}
