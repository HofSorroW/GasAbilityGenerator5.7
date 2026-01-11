@echo off
REM Quick build script for GasAbilityGenerator plugin
REM Usage: build.bat [ubt|sln|run|all]

setlocal enabledelayedexpansion

set PROJECT_ROOT=C:\Unreal Projects\NP22Beta
set UE_ROOT=C:\Program Files\Epic Games\UE_5.6
set UBT="%UE_ROOT%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
set EDITOR="%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe"
set LOG_DIR=%PROJECT_ROOT%\Plugins\GasAbilityGenerator\Tools\Logs

if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"

set ACTION=%1
if "%ACTION%"=="" set ACTION=ubt

echo ============================================
echo GasAbilityGenerator Build Script
echo Action: %ACTION%
echo ============================================

if "%ACTION%"=="ubt" goto :build_ubt
if "%ACTION%"=="sln" goto :build_sln
if "%ACTION%"=="run" goto :run_editor
if "%ACTION%"=="all" goto :build_all
if "%ACTION%"=="errors" goto :show_errors

echo Unknown action: %ACTION%
echo Usage: build.bat [ubt^|sln^|run^|all^|errors]
exit /b 1

:build_ubt
echo Building with UnrealBuildTool...
%UBT% NP22BetaEditor Win64 Development "-Project=%PROJECT_ROOT%\NP22Beta.uproject" > "%LOG_DIR%\ubt_output.log" 2>&1
set BUILD_RESULT=%ERRORLEVEL%
if %BUILD_RESULT% neq 0 (
    echo [ERROR] UBT Build FAILED
    echo.
    echo === BUILD ERRORS ===
    findstr /i "error" "%LOG_DIR%\ubt_output.log"
    echo === END ERRORS ===
    exit /b %BUILD_RESULT%
)
echo [SUCCESS] UBT Build completed
exit /b 0

:build_sln
echo Building solution with MSBuild...
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do set MSBUILD=%%i
"%MSBUILD%" "%PROJECT_ROOT%\NP22Beta.sln" /t:Build /p:Configuration="Development Editor" /p:Platform=Win64 /verbosity:minimal > "%LOG_DIR%\msbuild_output.log" 2>&1
set BUILD_RESULT=%ERRORLEVEL%
if %BUILD_RESULT% neq 0 (
    echo [ERROR] MSBuild FAILED
    echo.
    echo === BUILD ERRORS ===
    findstr /i "error" "%LOG_DIR%\msbuild_output.log"
    echo === END ERRORS ===
    exit /b %BUILD_RESULT%
)
echo [SUCCESS] MSBuild completed
exit /b 0

:run_editor
echo Launching Unreal Editor...
start "" %EDITOR% "%PROJECT_ROOT%\NP22Beta.uproject"
echo Editor launched
exit /b 0

:build_all
call :build_ubt
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
call :run_editor
exit /b 0

:show_errors
echo === Recent UBT Errors ===
if exist "%LOG_DIR%\ubt_output.log" findstr /i "error" "%LOG_DIR%\ubt_output.log"
echo.
echo === Recent MSBuild Errors ===
if exist "%LOG_DIR%\msbuild_output.log" findstr /i "error" "%LOG_DIR%\msbuild_output.log"
exit /b 0
