@echo off
setlocal enabledelayedexpansion

echo ============================================================
echo   SwarmCudaProject - GPU Auto-Detect Launcher
echo ============================================================
echo.

:: ── Configuration ────────────────────────────────────────────────────────────
set "EXE=%~dp0build\SwarmCudaProject.exe"
set "ZLUDA_DIR=%~dp0zluda"
set "ZLUDA_CACHE=%ZLUDA_DIR%\cache"
set "ZLUDA_ZIP=%ZLUDA_DIR%\zluda-windows.zip"
set "ZLUDA_WITH=%ZLUDA_CACHE%\zluda\zluda.exe"
if not exist "%ZLUDA_WITH%" set "ZLUDA_WITH=%ZLUDA_CACHE%\zluda\zluda.exe"

:: ── Check the executable exists ───────────────────────────────────────────────
if not exist "%EXE%" (
    echo [ERROR] Build not found at: %EXE%
    echo         Run CMake to build the project first.
    pause
    exit /b 1
)

:: ── Detect GPU via PowerShell (wmic is deprecated on Windows 11) ──────────────
echo [*] Detecting GPU(s)...
set "IS_AMD=0"

for /f "usebackq tokens=1,2 delims=|" %%N in (`powershell -NoProfile -Command ^
    "Get-WmiObject Win32_VideoController | ForEach-Object { $_.Name + '|' + $_.AdapterCompatibility }"`) do (
    set "GPU_NAME=%%N"
    set "GPU_COMPAT=%%O"
    echo     Found: !GPU_NAME! ^(!GPU_COMPAT!^)
    echo !GPU_NAME!!GPU_COMPAT! | findstr /i "Radeon AMD ATI RDNA Advanced.Micro.Devices" >nul 2>&1
    if not errorlevel 1 set "IS_AMD=1"
)
echo.

:: ── Route to correct path ─────────────────────────────────────────────────────
if "%IS_AMD%"=="1" goto :amd_path
echo [*] NVIDIA or other GPU detected. Running with native CUDA.
goto :run_direct

:: ════════════════════════════════════════════════════════════════════════════
:amd_path
:: ════════════════════════════════════════════════════════════════════════════
echo [*] AMD GPU detected. Using ZLUDA for CUDA-on-ROCm compatibility.
echo.

:: Check if ZLUDA is already downloaded
if exist "%ZLUDA_WITH%" goto :run_with_zluda

:: ── Download ZLUDA ────────────────────────────────────────────────────────────
echo [*] ZLUDA not found locally. Fetching from GitHub...

if not exist "%ZLUDA_DIR%" mkdir "%ZLUDA_DIR%"
if not exist "%ZLUDA_CACHE%" mkdir "%ZLUDA_CACHE%"

:: Get latest ZLUDA pre-release Windows release URL from GitHub API
echo [*] Querying GitHub for latest ZLUDA pre-release...
set "ZLUDA_URL="
for /f "usebackq tokens=*" %%U in (`powershell -NoProfile -Command ^
    "try { $r = Invoke-RestMethod 'https://api.github.com/repos/vosen/ZLUDA/releases'; ($r | Where-Object { $_.prerelease } | Select-Object -First 1).assets | Where-Object { $_.name -like '*windows*' -and $_.name -like '*.zip' } | Select-Object -First 1 -ExpandProperty browser_download_url } catch { '' }"`) do (
    set "ZLUDA_URL=%%U"
)

:: Fallback to known v5 URL if API call failed
if "!ZLUDA_URL!"=="" (
    echo [WARN] GitHub API unavailable. Using fallback URL for ZLUDA v6...
    set "ZLUDA_URL=https://github.com/vosen/ZLUDA/releases/download/v6-preview.67/zluda-windows-9854942.zip"
)
echo [*] Downloading: !ZLUDA_URL!

powershell -NoProfile -Command "Invoke-WebRequest -Uri '!ZLUDA_URL!' -OutFile '%ZLUDA_ZIP%' -UseBasicParsing"

if not exist "%ZLUDA_ZIP%" (
    echo [ERROR] Download failed. Check internet connection.
    echo         You can manually download ZLUDA from:
    echo         https://github.com/vosen/ZLUDA/releases/latest
    echo         Extract the Windows zip to: %ZLUDA_CACHE%
    pause
    exit /b 1
)

:: ── Extract ───────────────────────────────────────────────────────────────────
echo [*] Extracting ZLUDA...
powershell -NoProfile -Command "Expand-Archive -Path '%ZLUDA_ZIP%' -DestinationPath '%ZLUDA_CACHE%' -Force"
del "%ZLUDA_ZIP%" 2>nul

if not exist "%ZLUDA_WITH%" (
    echo [ERROR] ZLUDA extraction failed - zluda.exe not found.
    echo         Expected at: %ZLUDA_WITH%
    echo         Actual contents of %ZLUDA_CACHE%:
    dir /s /b "%ZLUDA_CACHE%\*.exe" 2>nul
    pause
    exit /b 1
)
echo [*] ZLUDA v5 ready.
echo.

:: ════════════════════════════════════════════════════════════════════════════
:run_with_zluda
:: ZLUDA v5 uses zluda.exe as a launcher that injects CUDA-to-ROCm
:: translation at runtime. No DLL copying needed.
:: ════════════════════════════════════════════════════════════════════════════
echo [*] Launching with ZLUDA (AMD ROCm backend)...
echo     Launcher: %ZLUDA_WITH%
echo     EXE:      %EXE%
echo.
echo [NOTE] Ensure AMD Adrenalin drivers 24.x+ are installed for ROCm support.
echo.

"%ZLUDA_WITH%" -- "%EXE%" %*
set "EXIT_CODE=%ERRORLEVEL%"
goto :report_exit

:: ════════════════════════════════════════════════════════════════════════════
:run_direct
:: ════════════════════════════════════════════════════════════════════════════
echo [*] Launching directly (native CUDA)...
echo     EXE: %EXE%
echo.

"%EXE%" %*
set "EXIT_CODE=%ERRORLEVEL%"

:: ════════════════════════════════════════════════════════════════════════════
:report_exit
:: ════════════════════════════════════════════════════════════════════════════
echo.
if "%EXIT_CODE%"=="0" (
    echo [OK] Process exited cleanly ^(code 0^).
) else (
    echo [WARN] Process exited with code %EXIT_CODE%.
    if "%IS_AMD%"=="1" (
        echo.
        echo  Troubleshooting tips for AMD + ZLUDA:
        echo    1. Ensure AMD Adrenalin 24.x+ drivers are installed
        echo    2. ROCm-capable GPU required ^(RX 5000 series or newer^)
        echo    3. Delete the zluda\ folder to force a fresh ZLUDA download
        echo    4. Check ZLUDA docs: https://zluda.readthedocs.io
    )
)

endlocal
pause
