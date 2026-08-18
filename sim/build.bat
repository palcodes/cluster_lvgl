@echo off
REM Build the Windows simulator with MinGW-w64 gcc.
REM
REM   sim\build.bat                     uses ..\lvgl_src
REM   set LVGL_DIR=C:\path\to\lvgl  &  sim\build.bat
REM
REM Needs gcc on PATH.  With MSYS2 that is C:\msys64\mingw64\bin.

setlocal enabledelayedexpansion

set "ROOT=%~dp0.."
if "%LVGL_DIR%"=="" set "LVGL_DIR=%ROOT%\..\gui\lvgl"

if not exist "%LVGL_DIR%\lvgl.h" (
    echo LVGL not found at %LVGL_DIR%
    echo   git clone --depth 1 -b release/v9.4 https://github.com/lvgl/lvgl.git "%LVGL_DIR%"
    exit /b 1
)

where gcc >nul 2>nul
if errorlevel 1 (
    echo gcc is not on PATH.  With MSYS2 add C:\msys64\mingw64\bin
    exit /b 1
)

if not exist "%ROOT%\build" mkdir "%ROOT%\build"
set "RSP=%ROOT%\build\sources.rsp"
if exist "%RSP%" del /q "%RSP%"

REM Collect every source into a response file - the command line would
REM otherwise blow past the 8191 character limit.
for /r "%LVGL_DIR%\src" %%f in (*.c) do echo "%%f">>"%RSP%"
for %%f in ("%ROOT%\fonts\cl_font_*.c") do echo "%ROOT%\fonts\%%~nxf">>"%RSP%"
for %%f in ("%ROOT%\icons\cl_*.c") do echo "%ROOT%\icons\%%~nxf">>"%RSP%"
echo "%ROOT%\ui\cl_screen.c">>"%RSP%"
echo "%ROOT%\ui\cl_fonts.c">>"%RSP%"
echo "%ROOT%\ui\cl_pool.c">>"%RSP%"
echo "%ROOT%\app\cluster_data.c">>"%RSP%"
echo "%ROOT%\sim\main_win32.c">>"%RSP%"

echo Building...
gcc -O2 -DLV_CONF_INCLUDE_SIMPLE ^
    -I"%ROOT%\sim" -I"%LVGL_DIR%" -I"%ROOT%\ui" -I"%ROOT%\app" ^
    @"%RSP%" ^
    -lgdi32 -luser32 -mwindows ^
    -o "%ROOT%\build\cluster.exe"

if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

echo.
echo   %ROOT%\build\cluster.exe
echo.
endlocal
