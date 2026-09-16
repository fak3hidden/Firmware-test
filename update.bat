@echo off
rem ============================================================
rem  T-Embed CC1101 Firmware Updater - Flipper Zero style
rem  - clones the repo if missing
rem  - pulls the newest firmware source (git pull --ff-only)
rem  - optional: builds and flashes with PlatformIO
rem ============================================================
setlocal
title T-Embed CC1101 Firmware Updater
mode con: cols=82 lines=34 >nul 2>nul
cd /d "%~dp0"

rem --- ANSI colors (Windows 10/11) ---
for /F %%a in ('echo prompt $E ^| cmd') do set "ESC=%%a"
set "ORG=%ESC%[38;2;255;130;0m"
set "WHT=%ESC%[97m"
set "GRY=%ESC%[90m"
set "GRN=%ESC%[92m"
set "RED=%ESC%[91m"
set "RST=%ESC%[0m"

set "REPO=T-Embed-CC1101"
set "GURL=https://github.com/Xinyuan-LilyGO/T-Embed-CC1101.git"

cls
call :banner

where git >nul 2>nul
if errorlevel 1 goto :nogit

rem ============================================================
rem  Stage 0: self-update - pull THIS repository (updater,
rem  theme patch, previews) first, then relaunch with fresh code.
rem ============================================================
if /i "%~1"=="noself" goto :selfdone
if not exist ".git" goto :selfdone
echo(%GRY%  Checking for updates to this updater...%RST%
git fetch origin --quiet 2>nul
if errorlevel 1 goto :selfdone
set "BR0="
for /f %%b in ('git rev-parse --abbrev-ref HEAD 2^>nul') do set "BR0=%%b"
if not defined BR0 goto :selfdone
set "SELF_BEHIND="
for /f %%c in ('git rev-list --count HEAD..origin/%BR0% 2^>nul') do set "SELF_BEHIND=%%c"
if not defined SELF_BEHIND goto :selfdone
echo %SELF_BEHIND%| findstr /r "^[0-9][0-9]*$" >nul || set "SELF_BEHIND=0"
if "%SELF_BEHIND%"=="0" (
    echo(%GRN%  Updater repo: up to date.%RST%
    goto :selfdone
)
echo(%ORG%  Updater repo: %SELF_BEHIND% new commit^(s^) - pulling...%RST%
git pull --ff-only >nul 2>nul
if errorlevel 1 goto :selffail
echo(%GRN%  Updater refreshed. Restarting with the new code...%RST%
echo.
call "%~f0" noself & exit /b 0

:selffail
echo(%RED%  Self-update failed ^(local changes blocking fast-forward?^)%RST%
echo(%GRY%  Continuing with the current scripts - try: git stash%RST%

:selfdone

if not exist "%REPO%\.git" goto :askclone
goto :fetch

rem ------------------------------------------------------------
:askclone
echo(%GRY%  +---------------------------------------------------------------+%RST%
echo(%GRY%  ^|%RST%  %WHT%%REPO%%RST% not found here.                                   %GRY%^|%RST%
echo(%GRY%  +---------------------------------------------------------------+%RST%
echo.
choice /c YN /n /m "  %ORG%Clone it from GitHub now? [Y/N]%RST% "
if errorlevel 2 goto :bye
echo.
echo(%GRY%  Cloning %GURL% ...%RST%
git clone "%GURL%"
if errorlevel 1 goto :clonefail
echo(%GRN%  Clone complete. The dolphin is pleased.%RST%
echo.
goto :fetch

rem ------------------------------------------------------------
:fetch
if not exist "%REPO%" md "%REPO%" >nul 2>nul
pushd "%REPO%" >nul 2>&1
if errorlevel 1 goto :nofolder

echo(%GRY%  Contacting the mothership for updates...%RST%
git fetch origin --quiet
if errorlevel 1 (
    popd
    goto :offline
)

for /f %%b in ('git rev-parse --abbrev-ref HEAD 2^>nul') do set "BR=%%b"
if not defined BR set "BR=main"
set "BEHIND="
for /f %%c in ('git rev-list --count HEAD..origin/%BR% 2^>nul') do set "BEHIND=%%c"
if not defined BEHIND set "BEHIND=0"
echo %BEHIND%| findstr /r "^[0-9][0-9]*$" >nul || set "BEHIND=0"

echo(%GRY%  Branch :%RST% %WHT%%BR%%RST%
if "%BEHIND%"=="0" (
    echo(%GRN%  Status : already up to date.%RST%
    echo.
    call :dolphin_ok
) else (
    echo(%ORG%  Status : %BEHIND% new commit^(s^) waiting%RST%
    echo.
    echo(%GRY%  --- incoming firmware changelog ---------------------------------%RST%
    git --no-pager log --oneline HEAD..origin/%BR%
    echo(%GRY%  ----------------------------------------------------------------%RST%
    echo.
    choice /c YN /n /m "  %ORG%Install update now? [Y/N]%RST% "
    if errorlevel 2 (
        echo(%GRY%  Skipped. The dolphin waits patiently.%RST%
    ) else (
        echo.
        echo(%ORG%  Pulling new firmware...%RST%
        git pull --ff-only
        if errorlevel 1 (
            popd
            goto :pullfail
        )
        echo(%GRN%  Update installed. Flipper-tastic!%RST%
        echo.
        call :dolphin_ok
    )
)

for /f %%v in ('git --no-pager log -1 --format^=%%h 2^>nul') do set "VER=%%v"
echo(%GRY%  Source at commit:%RST% %WHT%%VER%%RST%
popd
call :apply_theme
goto :buildask

rem ------------------------------------------------------------
:apply_theme
if not exist "flipper-style.patch" exit /b
pushd "%REPO%" >nul 2>&1
git apply -R --check "..\flipper-style.patch" >nul 2>nul
if not errorlevel 1 (
    echo(%GRY%  Flipper theme already applied.%RST%
    popd
    exit /b
)
echo.
echo(%ORG%  Applying the Flipper Zero theme patch...%RST%
git apply --3way "..\flipper-style.patch" >nul 2>nul
if errorlevel 1 (
    echo(%RED%  Theme patch failed ^(upstream changed a themed file^).%RST%
    echo(%GRY%  Continuing with the stock look.%RST%
) else (
    echo(%GRN%  Flipper Zero theme applied.%RST%
)
popd
exit /b

rem ------------------------------------------------------------
:buildask
echo.
where pio >nul 2>nul
if errorlevel 1 (
    echo(%GRY%  PlatformIO not found - skipping build step.%RST%
    echo(%GRY%  Install it with: pip install platformio%RST%
    goto :bye
)

choice /c YN /n /m "  %ORG%Build the firmware with PlatformIO? [Y/N]%RST% "
if errorlevel 2 goto :bye
echo.
pio run -d "%REPO%" -e T_Embed_CC1101
if errorlevel 1 goto :buildfail
echo.
echo(%GRN%  Build OK. Firmware image is ready.%RST%
echo.
choice /c YN /n /m "  %ORG%Flash it to the T-Embed now? [Y/N]%RST% "
if errorlevel 2 goto :bye
echo.
echo(%ORG%  Flashing... keep the device connected.%RST%
pio run -d "%REPO%" -e T_Embed_CC1101 -t upload
if errorlevel 1 goto :flashfail
echo(%GRN%  Flash complete. Device updated - the dolphin salutes you.%RST%
goto :bye

rem ------------------------------------------------------------
:dolphin_ok
echo(%ORG%           _.--._%RST%
echo(%ORG%        .-'      '-.%RST%
echo(%ORG%       /   .-~~-.   \%RST%
echo(%ORG%      ^|   /      \   ^|      %WHT%All good!%RST%
echo(%ORG%      ^|   \   o  ^|   ^|%RST%
echo(%ORG%       \   \__..-'   /%RST%
echo(%ORG%        '.         .'%RST%
echo(%ORG%          '-....-'%RST%
exit /b

:banner
echo(%ORG%           _.--._%RST%
echo(%ORG%        .-'      '-.%RST%
echo(%ORG%       /   .-~~-.   \      %WHT%T-EMBED CC1101%RST%
echo(%ORG%      ^|   /      \   ^|     %WHT%FIRMWARE UPDATER%RST%
echo(%ORG%      ^|   \   o  ^|   ^|     %GRY%Flipper Zero style edition%RST%
echo(%ORG%       \   \__..-'   /%RST%
echo(%ORG%        '.         .'%RST%
echo(%ORG%          '-....-'       %GRY%by LILYGO / GitHub update%RST%
echo.
goto :eof

:nogit
echo(%RED%  ERROR: git was not found on PATH.%RST%
echo(%WHT%  Install Git for Windows from https://git-scm.com/downloads%RST%
goto :fail

:nofolder
echo(%RED%  ERROR: folder %REPO% is not accessible.%RST%
goto :fail

:offline
echo(%RED%  ERROR: could not reach GitHub (offline or network blocked).%RST%
goto :fail

:clonefail
echo(%RED%  ERROR: git clone failed.%RST%
goto :fail

:pullfail
echo(%RED%  ERROR: update failed (local changes blocking a fast-forward?).%RST%
echo(%GRY%  Try: git stash    then run this updater again.%RST%
goto :fail

:buildfail
echo(%RED%  ERROR: firmware build failed. Scroll up for compiler errors.%RST%
goto :fail

:flashfail
echo(%RED%  ERROR: flashing failed. Check USB cable, port and drivers.%RST%
goto :fail

:fail
echo.
echo(%ORG%          '-....-'       %GRY%the dolphin is sad%RST%
goto :end

:bye
echo.
echo(%GRY%  Done. Press any key to close.%RST%

:end
echo.
pause >nul
exit /b 0
