@echo off
setlocal EnableExtensions EnableDelayedExpansion
chcp 65001 >nul 2>&1

rem CortexOS dependency installer for Windows.
rem MSYS2 and WSL are the supported full-build environments. Native
rem Chocolatey/winget installation is intentionally only a partial fallback.

set "SCRIPT_DIR=%~dp0"
set "YES=0"
set "CHECK_ONLY=0"
set "METHOD="
set /a MISSING=0

:parse_args
if "%~1"=="" goto args_done
if /i "%~1"=="-y" set "YES=1"& shift& goto parse_args
if /i "%~1"=="--yes" set "YES=1"& shift& goto parse_args
if /i "%~1"=="--check" set "CHECK_ONLY=1"& shift& goto parse_args
if /i "%~1"=="--help" goto help
if /i "%~1"=="-h" goto help
if /i "%~1"=="--method=msys2" set "METHOD=msys2"& shift& goto parse_args
if /i "%~1"=="--method=wsl" set "METHOD=wsl"& shift& goto parse_args
if /i "%~1"=="--method=choco" set "METHOD=choco"& shift& goto parse_args
if /i "%~1"=="--method=winget" set "METHOD=winget"& shift& goto parse_args
echo [!] Unknown option: %~1
exit /b 2

:args_done
call :detect_environment
call :print_banner
call :check_dependencies
if "%CHECK_ONLY%"=="1" (
    if %MISSING%==0 exit /b 0
    echo.
    echo [!] %MISSING% dependency group(s) are missing.
    exit /b 1
)

if %MISSING%==0 (
    echo.
    echo [+] All detected Windows dependencies are available.
    echo     For ARM builds, WSL or a Linux CI environment is recommended.
    goto finish
)

if not defined METHOD (
    if "%YES%"=="1" (
        if "%HAS_WSL%"=="1" (set "METHOD=wsl") else if "%HAS_MSYS2%"=="1" (set "METHOD=msys2")
    ) else (
        call :choose_method
    )
)

if /i "%METHOD%"=="msys2" call :install_msys2 & goto finish
if /i "%METHOD%"=="wsl" call :install_wsl & goto finish
if /i "%METHOD%"=="choco" call :install_choco & goto finish
if /i "%METHOD%"=="winget" call :install_winget & goto finish

echo.
echo [!] No usable installation method was selected.
echo     Use --method=msys2, --method=wsl, --method=choco or --method=winget.
exit /b 1

:print_banner
 echo.
 echo ================================================================
 echo   CortexOS dependency installer - Windows
 echo ================================================================
 echo   MSYS2 and WSL provide the complete toolchain.
 echo.
exit /b 0

:detect_environment
set "HAS_MSYS2=0"
set "HAS_WSL=0"
set "HAS_CHOCO=0"
set "HAS_WINGET=0"
set "MSYS2_ROOT="
set "WSL_DISTRO="

if defined MSYSTEM set "HAS_MSYS2=1"
if defined MSYS2_PATH_TYPE set "HAS_MSYS2=1"
if exist "C:\msys64\usr\bin\bash.exe" (
    set "HAS_MSYS2=1"
    set "MSYS2_ROOT=C:\msys64"
)
if exist "C:\msys32\usr\bin\bash.exe" (
    set "HAS_MSYS2=1"
    set "MSYS2_ROOT=C:\msys32"
)
if defined MSYS2_ROOT goto detect_wsl
:detect_wsl
where wsl.exe >nul 2>&1
if not errorlevel 1 (
    for /f "delims=" %%D in ('wsl.exe --list --quiet 2^>nul') do if not defined WSL_DISTRO set "WSL_DISTRO=%%D"
    if defined WSL_DISTRO set "HAS_WSL=1"
)
where choco.exe >nul 2>&1
if not errorlevel 1 set "HAS_CHOCO=1"
where winget.exe >nul 2>&1
if not errorlevel 1 set "HAS_WINGET=1"
exit /b 0

:check_dependencies
 echo [*] Available tools:
 call :check_tool "GNU C compiler (gcc)" gcc
 call :check_tool "GNU binutils (ld)" ld
 call :check_make
 call :check_tool "NASM" nasm
 call :check_tool "xorriso" xorriso
 call :check_tool "mtools (mformat)" mformat
 call :check_python
 call :check_tool "QEMU x86_64" qemu-system-x86_64
 call :check_tool "QEMU ARM32" qemu-system-arm
 call :check_tool "QEMU AArch64" qemu-system-aarch64
 call :check_tool "Rust (rustc)" rustc
 call :check_tool "GRUB image builder" grub-mkrescue
 echo.
 echo [*] Installation methods:
 echo     MSYS2:      %HAS_MSYS2%
 echo     WSL:        %HAS_WSL%  %WSL_DISTRO%
 echo     Chocolatey: %HAS_CHOCO%
 echo     winget:     %HAS_WINGET%
exit /b 0

:check_tool
where %~2 >nul 2>&1
if errorlevel 1 (echo [!] %~1 - missing& set /a MISSING+=1) else echo [+] %~1
exit /b 0

:check_make
where make >nul 2>&1
if not errorlevel 1 (echo [+] GNU Make& exit /b 0)
where mingw32-make >nul 2>&1
if not errorlevel 1 (echo [+] GNU Make (mingw32-make)& exit /b 0)
echo [!] GNU Make - missing
set /a MISSING+=1
exit /b 0

:check_python
where python >nul 2>&1
if not errorlevel 1 (echo [+] Python 3& exit /b 0)
where python3 >nul 2>&1
if not errorlevel 1 (echo [+] Python 3& exit /b 0)
echo [!] Python 3 - missing
set /a MISSING+=1
exit /b 0

:choose_method
 echo.
 echo [*] Choose an installation method:
 if "%HAS_MSYS2%"=="1" echo     1. MSYS2 (recommended for native Windows builds)
 if "%HAS_WSL%"=="1" echo     2. WSL (recommended for ARM cross-compilers)
 if "%HAS_CHOCO%"=="1" echo     3. Chocolatey (partial)
 if "%HAS_WINGET%"=="1" echo     4. winget (partial)
 echo     5. Cancel
 set /p "CHOICE=Selection [1-5]: "
 if "%CHOICE%"=="1" if "%HAS_MSYS2%"=="1" set "METHOD=msys2"
 if "%CHOICE%"=="2" if "%HAS_WSL%"=="1" set "METHOD=wsl"
 if "%CHOICE%"=="3" if "%HAS_CHOCO%"=="1" set "METHOD=choco"
 if "%CHOICE%"=="4" if "%HAS_WINGET%"=="1" set "METHOD=winget"
 if "%CHOICE%"=="5" exit /b 0
exit /b 0

:install_msys2
if not defined MSYS2_ROOT set "MSYS2_ROOT=C:\msys64"
if not exist "%MSYS2_ROOT%\usr\bin\bash.exe" (
    echo [!] MSYS2 bash was not found at %MSYS2_ROOT%.
    echo     Install MSYS2, open its UCRT64 terminal, and run this script again.
    exit /b 1
)
echo [*] Installing native build packages through MSYS2 pacman...
"%MSYS2_ROOT%\usr\bin\bash.exe" -lc "pacman -S --needed --noconfirm base-devel gcc binutils make nasm xorriso mtools python qemu rust"
if errorlevel 1 (
    echo [!] MSYS2 installation failed.
    exit /b 1
)
echo [+] MSYS2 packages installed.
echo     Restart the MSYS2 terminal so PATH changes take effect.
exit /b 0

:install_wsl
if "%HAS_WSL%"=="0" (
    echo [!] No WSL distribution was found.
    echo     Install one from an elevated terminal with: wsl --install -d Ubuntu
    exit /b 1
)
if not defined WSL_DISTRO set "WSL_DISTRO=Ubuntu"
rem Convert the local script path using WSL itself; this also handles spaces.
set "WSL_SCRIPT="
for /f "delims=" %%P in ('wsl.exe -d "%WSL_DISTRO%" -- wslpath -a "%SCRIPT_DIR%install-deps.sh" 2^>nul') do set "WSL_SCRIPT=%%P"
if not defined WSL_SCRIPT (
    echo [!] Could not convert the local script path for WSL.
    exit /b 1
)
echo [*] Running the local installer inside WSL (%WSL_DISTRO%)...
wsl.exe -d "%WSL_DISTRO%" -- bash "%WSL_SCRIPT%" --yes
if errorlevel 1 (
    echo [!] WSL dependency installation failed.
    exit /b 1
)
echo [+] WSL dependencies installed.
exit /b 0

:install_choco
if "%HAS_CHOCO%"=="0" exit /b 1
echo [*] Installing the available native tools through Chocolatey...
choco install -y make python3 qemu nasm
if errorlevel 1 echo [!] Chocolatey reported an error.
echo [~] Chocolatey does not provide the complete GNU/GRUB/ARM environment.
echo     Use MSYS2 or WSL for a full CortexOS build.
exit /b 0

:install_winget
if "%HAS_WINGET%"=="0" exit /b 1
echo [*] Installing Python and QEMU through winget...
winget install --id Python.Python.3.12 -e --accept-source-agreements --accept-package-agreements
winget install --id SoftwareFreedomConservancy.QEMU -e --accept-source-agreements --accept-package-agreements
if errorlevel 1 echo [!] winget reported an error.
echo [~] winget is a partial fallback; use MSYS2 or WSL for gcc, GRUB and ARM.
exit /b 0

:finish
echo.
echo [*] Next steps:
echo     make clean ^&^& make
echo     make iso
echo     make run
endlocal
exit /b 0

:help
echo Usage: tools\setup\install-deps.bat [options]
echo.
echo Options:
echo   --yes                 Install without prompting (WSL preferred, then MSYS2)
echo   --check               Only check tools; returns exit code 1 when missing
 echo  --method=msys2       Use MSYS2
 echo  --method=wsl         Use WSL
 echo  --method=choco       Use Chocolatey (partial)
 echo  --method=winget      Use winget (partial)
echo   -h, --help            Show this help
exit /b 0
