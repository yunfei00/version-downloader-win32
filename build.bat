@echo off
setlocal

if not exist build mkdir build

for /f %%i in ('git rev-parse --short HEAD 2^>nul') do set GIT_COMMIT=%%i
if "%GIT_COMMIT%"=="" set GIT_COMMIT=dev

cl /std:c++17 /EHsc /O2 /DUNICODE /D_UNICODE /DWIN32 /D_WINDOWS /DNDEBUG /DBUILD_COMMIT=L"%GIT_COMMIT%" ^
  src\main.cpp src\app.cpp src\ui.cpp src\csv_parser.cpp src\logger.cpp src\download_worker.cpp src\version.rc ^
  /Fe:build\version-downloader.exe ^
  comctl32.lib winhttp.lib comdlg32.lib shell32.lib
if errorlevel 1 (
  echo Build failed.
  exit /b 1
)

copy /Y README.md build\README.md >nul
copy /Y sample_versions.csv build\sample_versions.csv >nul

echo Build completed: build\version-downloader.exe
endlocal
