#include "pch.h"
#include "string_utils.h"

std::wstring Utf8ToWide(const std::string& input) {
    if (input.empty()) {
        return L"";
    }

    int wideLen = MultiByteToWideChar(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), nullptr, 0);
    if (wideLen <= 0) {
        return L"";
    }

    std::wstring output(static_cast<size_t>(wideLen), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), output.data(), wideLen);
    return output;
}

std::string WideToUtf8(const std::wstring& input) {
    if (input.empty()) {
        return "";
    }

    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), nullptr, 0, nullptr, nullptr);
    if (utf8Len <= 0) {
        return "";
    }

    std::string output(static_cast<size_t>(utf8Len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), output.data(), utf8Len, nullptr, nullptr);
    return output;
}

std::wstring FormatBytes(unsigned long long bytes){ const wchar_t* u[]={L"B",L"KB",L"MB",L"GB"}; double v=(double)bytes; int i=0; while(v>=1024.0&&i<3){v/=1024.0;++i;} wchar_t b[64]; swprintf(b,64,L"%.2f %s",v,u[i]); return b; }
