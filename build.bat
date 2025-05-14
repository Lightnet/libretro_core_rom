@echo off
setlocal

set PROJECT_DIR=%CD%
set BUILD_DIR=%PROJECT_DIR%\build
set RETROARCH_DIR=D:\dev\RetroArch-Win64
set CORE_NAME=hello_world_core.dll

mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug

:: Copy the DLL and font to RetroArch cores directory
@REM echo Copying %CORE_NAME% and fonts to RetroArch...
@REM copy "%BUILD_DIR%\Debug\%CORE_NAME%" "%RETROARCH_DIR%\cores\"
@REM if %ERRORLEVEL% neq 0 (
@REM     echo Failed to copy DLL to RetroArch!
@REM     exit /b %ERRORLEVEL%
@REM )

endlocal