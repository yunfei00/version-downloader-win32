@echo off
setlocal

if /I "%VSCMD_VER%"=="" (
  if defined VSINSTALLDIR (
    if exist "%VSINSTALLDIR%\VC\Auxiliary\Build\vcvars64.bat" (
      call "%VSINSTALLDIR%\VC\Auxiliary\Build\vcvars64.bat"
    )
  )

  if /I "%VSCMD_VER%"=="" (
    if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
      for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_PATH=%%i"
      if defined VS_PATH if exist "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"
    )
  )
)

where cl >nul 2>nul
if errorlevel 1 (
  echo ERROR: cl.exe not found. Please open "x64 Native Tools Command Prompt for VS" or install Build Tools.
  exit /b 1
)

if not exist build mkdir build

del /q build\*.obj build\*.res build\version-downloader.exe 2>nul

for /f %%i in ('git rev-parse --short HEAD 2^>nul') do set GIT_COMMIT=%%i
if "%GIT_COMMIT%"=="" set GIT_COMMIT=dev

cl /nologo /std:c++17 /EHsc /O2 /MD /DUNICODE /D_UNICODE /DWIN32 /D_WINDOWS /DNDEBUG /DBUILD_COMMIT=L"%GIT_COMMIT%" /c ^
  src\main.cpp src\app.cpp src\ui.cpp src\csv_parser.cpp src\logger.cpp src\download_worker.cpp /Fo:build\
if errorlevel 1 (
  echo Build failed while compiling C++ sources.
  exit /b 1
)

rc /nologo /d NDEBUG /fo build\version.res src\version.rc
if errorlevel 1 (
  echo Build failed while compiling resource file: src\version.rc
  exit /b 1
)

link /nologo /OUT:build\version-downloader.exe build\main.obj build\app.obj build\ui.obj build\csv_parser.obj build\logger.obj build\download_worker.obj build\version.res ^
  comctl32.lib winhttp.lib comdlg32.lib shell32.lib user32.lib gdi32.lib advapi32.lib ole32.lib
if errorlevel 1 (
  echo Build failed while linking executable.
  exit /b 1
)

copy /Y README.md build\README.md >nul
copy /Y sample_versions.csv build\sample_versions.csv >nul

echo Build completed: build\version-downloader.exe
endlocal
