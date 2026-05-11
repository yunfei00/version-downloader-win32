#pragma once

#include <string>
#include <vector>

struct RemoteFileItem {
    std::wstring name;
    std::wstring url;
    unsigned long long size = 0;
    bool sizeKnown = false;
};

bool ScanHttpDirectory(const std::wstring& directoryUrl, std::vector<RemoteFileItem>& outFiles, std::wstring& error);
