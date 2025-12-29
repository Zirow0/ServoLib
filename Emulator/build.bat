@echo off
REM Скрипт для збірки емулятора ServoLib на Windows з MinGW
REM Автор: ServoCore Team
REM Дата: 2025

echo ======================================
echo ServoLib Emulator Build Script
echo ======================================
echo.

REM Перевірка наявності CMake
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake not found in PATH
    echo Please install CMake from https://cmake.org/download/
    exit /b 1
)

REM Перевірка наявності MinGW
where mingw32-make >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: MinGW make not found in PATH
    echo Please install MinGW-w64 or MSYS2
    exit /b 1
)

REM Створення директорії збірки
if not exist build (
    echo Creating build directory...
    mkdir build
) else (
    echo Build directory already exists
)

cd build

REM Генерація Makefile
echo.
echo Generating Makefiles with CMake...
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake generation failed
    cd ..
    exit /b 1
)

REM Компіляція проекту
echo.
echo Building project...
mingw32-make -j4
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed
    cd ..
    exit /b 1
)

echo.
echo ======================================
echo Build completed successfully!
echo ======================================
echo.
echo Executable: build\ServoLib_Emulator.exe
echo.
echo To run the emulator:
echo   cd build
echo   ServoLib_Emulator.exe
echo.

cd ..
