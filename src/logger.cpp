#include "pch.h"
#include "logger.h"

std::wstring BuildLogLine(const std::wstring& text) {
    SYSTEMTIME st{}; GetLocalTime(&st);
    wchar_t t[32]{};
    wsprintfW(t, L"[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);
    return std::wstring(t) + text + L"\r\n";
}
