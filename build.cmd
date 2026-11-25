@echo off
setlocal EnableExtensions DisableDelayedExpansion
set "generator=ninja"
set "architecture=x86"
set "configuration=MinSizeRel"
set "mode=release"
set "build_version="

:parse
if "%~1"=="" goto parsed
set "option=%~1"
if /i "%option%"=="version" goto use_split_version
if /i "%option:~0,8%"=="version=" goto use_version
if /i "%~1"=="ninja" goto use_ninja
if /i "%~1"=="vs" goto use_vs
if /i "%~1"=="x86" goto use_x86
if /i "%~1"=="debug" goto use_debug
if /i "%~1"=="release" goto use_release
if /i "%~1"=="help" goto help
echo Unknown build option: %~1
goto help_error

:use_ninja
set "generator=ninja"
shift
goto parse

:use_vs
set "generator=vs"
shift
goto parse

:use_x86
set "architecture=x86"
shift
goto parse

:use_debug
set "configuration=Debug"
set "mode=debug"
shift
goto parse

:use_release
set "configuration=MinSizeRel"
set "mode=release"
shift
goto parse

:use_version
set "build_version=%option:~8%"
if not defined build_version exit /b 1
shift
goto parse

:use_split_version
shift
if "%~1"=="" exit /b 1
set "build_version=%~1"
shift
goto parse

:parsed
set "source_dir=%~dp0"
set "build_dir=%source_dir%build\%generator%-%architecture%"
set "artifact_dir=%build_dir%\%configuration%"
set "output_dir=%source_dir%out\%architecture%\%mode%"

set "vswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%vswhere%" exit /b 1
for /f "usebackq tokens=*" %%I in (`"%vswhere%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "vs_dir=%%I"
if not defined vs_dir exit /b 1

where cmake.exe >nul 2>nul
if errorlevel 1 set "PATH=%vs_dir%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%PATH%"

if "%generator%"=="ninja" (
  call "%vs_dir%\VC\Auxiliary\Build\vcvarsall.bat" amd64_x86
  if errorlevel 1 exit /b %errorlevel%
)

if "%generator%"=="ninja" (
  cmake --preset ninja -B "%build_dir%" -DCHROME_PLUS_ARCH=x86 -DCHROME_PLUS_TARGET_XP=ON "-DCHROME_PLUS_BUILD_VERSION=%build_version%"
) else (
  cmake --preset vs -B "%build_dir%" -A Win32 -DCHROME_PLUS_ARCH=x86 -DCHROME_PLUS_TARGET_XP=ON "-DCHROME_PLUS_BUILD_VERSION=%build_version%"
)
if errorlevel 1 exit /b %errorlevel%

cmake --build "%build_dir%" --config %configuration% --parallel
if errorlevel 1 exit /b %errorlevel%

cmake -E make_directory "%output_dir%"
cmake -E copy_if_different "%artifact_dir%\version.dll" "%source_dir%src\chrome++.ini" "%output_dir%"
if errorlevel 1 exit /b %errorlevel%
if exist "%artifact_dir%\version.pdb" cmake -E copy_if_different "%artifact_dir%\version.pdb" "%output_dir%\version.pdb"

echo Build output: %output_dir%
exit /b 0

:help
echo Usage: build [ninja^|vs] [x86] [debug^|release] [version=VALUE]
echo Defaults: ninja x86 release
exit /b 0

:help_error
echo Usage: build [ninja^|vs] [x86] [debug^|release] [version=VALUE]
exit /b 1
