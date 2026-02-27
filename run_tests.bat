@echo off
setlocal EnableDelayedExpansion

:: ============================================================================
::  Voxel-Sim Architect  —  run_tests.bat
::  Runs the test binary built by CMake (MinGW single-config generator).
::  Binary   :  build\bin\VoxelSimTests.exe
::  Results  :  logs\test_<date>_<time>.log  (one new file per run)
:: ============================================================================

set "ROOT=%~dp0"
set "TEST_EXE=%ROOT%build\bin\VoxelSimTests.exe"
set "LOGS_DIR=%ROOT%logs"

echo.
echo  Voxel-Sim Architect  ^|  Test Runner
echo  ==============================================
echo  Binary  : %TEST_EXE%
echo  Logs    : %LOGS_DIR%\test_^<date^>_^<time^>.log
echo  ==============================================

:: Ensure logs directory exists
if not exist "%LOGS_DIR%" mkdir "%LOGS_DIR%"

:: Check binary exists
if not exist "%TEST_EXE%" (
    echo.
    echo  ERROR: Test binary not found at:
    echo         %TEST_EXE%
    echo.
    echo  Build it first:
    echo    cd build
    echo    cmake .. -G "MinGW Makefiles"
    echo    cmake --build . --target VoxelSimTests
    echo.
    exit /b 1
)

:: Run tests — the binary itself writes the structured log to logs/
"%TEST_EXE%"
set "RESULT=%ERRORLEVEL%"

:: Show which log file was just created
echo.
echo  ==============================================
for /f "delims=" %%F in ('dir /b /o-d "%LOGS_DIR%\test_*.log" 2^>nul') do (
    echo  Latest log :  %LOGS_DIR%\%%F
    goto :show_done
)
:show_done
echo  ==============================================
echo.

if "%RESULT%"=="0" (
    echo  All tests passed.
) else (
    echo  One or more tests FAILED — see log above for details.
)

exit /b %RESULT%

