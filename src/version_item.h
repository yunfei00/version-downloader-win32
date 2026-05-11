#pragma once

#include <string>

struct VersionItem {
    int index = 0;
    std::wstring name;
    std::wstring filename;
    std::wstring size;
    unsigned long long sizeBytes = 0;
    bool sizeKnown = false;
    std::wstring url;
    std::wstring status = L"等待";
};
