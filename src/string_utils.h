#pragma once

#include <string>

std::wstring Utf8ToWide(const std::string& input);
std::string WideToUtf8(const std::wstring& input);

std::wstring FormatBytes(unsigned long long bytes);
