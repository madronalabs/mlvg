@echo off
setlocal enabledelayedexpansion

REM CLAP Saw Demo Plugin Testing Script - Windows Version
REM Builds the plugin and runs clap-validator to ensure CLAP specification compliance

echo === CLAP Saw Demo Plugin Testing ===
echo Project Root: %~dp0..
echo Build Directory: %~dp0..\build
echo.

set PROJECT_ROOT=%~dp0..
set BUILD_DIR=%PROJECT_ROOT%\build
set VALIDATOR_DIR=%PROJECT_ROOT%\libs\clap-validator

REM Check if Rust is installed
where cargo >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] Rust/Cargo not found. Please install Rust from https://rustup.rs/
    exit /b 1
)

REM Step 1: Build clap-validator
echo [INFO] Building clap-validator...
cd /d "%VALIDATOR_DIR%"
cargo build --release
if %errorlevel% neq 0 (
    echo [ERROR] Failed to build clap-validator
    exit /b 1
)
echo [SUCCESS] clap-validator built successfully

REM Step 2: Build the plugin
echo [INFO] Building CLAP Saw Demo plugin...
cd /d "%PROJECT_ROOT%"

REM Configure CMake if needed
if not exist "%BUILD_DIR%" (
    echo [INFO] Configuring CMake build...
    cmake -B"%BUILD_DIR%" -DCMAKE_BUILD_TYPE=Release -DCSD_INCLUDE_GUI=FALSE
)

REM Build the plugin
cmake --build "%BUILD_DIR%"
if %errorlevel% neq 0 (
    echo [ERROR] Failed to build plugin
    exit /b 1
)
echo [SUCCESS] Plugin built successfully

REM Step 3: Locate the built plugin
set PLUGIN_PATH=%BUILD_DIR%\clap-saw-demo.clap
if not exist "%PLUGIN_PATH%" (
    echo [ERROR] Plugin not found at: %PLUGIN_PATH%
    echo [INFO] Available files in build directory:
    dir "%BUILD_DIR%"
    exit /b 1
)

echo [SUCCESS] Plugin found at: %PLUGIN_PATH%

REM Step 4: Run clap-validator
echo [INFO] Running CLAP specification validation...
echo.
echo === CLAP Validator Output ===

cd /d "%VALIDATOR_DIR%"
cargo run --release -- validate "%PLUGIN_PATH%" --only-failed
set VALIDATOR_RESULT=%errorlevel%

echo.
echo === Validation Summary ===

if %VALIDATOR_RESULT% equ 0 (
    echo [SUCCESS] ✓ Plugin successfully passed all CLAP specification tests
    echo [INFO] Your plugin is ready for testing in DAWs!
) else (
    echo [ERROR] ✗ Plugin failed CLAP specification validation
    echo [INFO] Please fix the issues above before proceeding
    echo.
    echo [INFO] For debugging a specific test, use:
    echo   cd "%VALIDATOR_DIR%"
    echo   cargo run --release -- list tests
    echo   cargo run --release -- validate --in-process --test-filter ^<test-name^> "%PLUGIN_PATH%"
)

echo.
echo [INFO] Testing complete
exit /b %VALIDATOR_RESULT% 