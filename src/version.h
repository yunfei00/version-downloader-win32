#pragma once

#define APP_NAME L"Version Downloader"
#define APP_VERSION L"0.2.0"

#define STRINGIZE2(x) #x
#define STRINGIZE(x) STRINGIZE2(x)
#define WIDEN2(x) L##x
#define WIDEN(x) WIDEN2(x)

#define BUILD_DATE WIDEN(__DATE__) L" " WIDEN(__TIME__)
#ifndef BUILD_COMMIT
#define BUILD_COMMIT L"dev"
#endif
