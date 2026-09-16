@echo off
rem ============================================================
rem  T-Embed CC1101 Firmware Builder - Flipper Zero style
rem  - applies the Flipper theme patch when present
rem  - compiles with PlatformIO (env T_Embed_CC1101)
rem  - flashes over USB with port auto-detect and retries
rem ============================================================
setlocal
title T-Embed CC1101 Firmware Builder
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
set "PIO_CMD="
set "TRIES=0"
set "PORT="
set "UPLOAD_ARGS="

cls
call :banner

rem --- the firmware source must be here ---
if not exist "%REPO%\platformio.ini" goto :norepo

rem --- apply the Flipper theme patch if we can ---
if exist "flipper-style.patch" (
    where git >nul 2>nul
    if not errorlevel 1 call :apply_theme
)

rem --- find or install PlatformIO ---
call :find_pio
if not defined PIO_CMD goto :nopio
goto :havepio

:nopio
echo(%RED%  PlatformIO was not found on this machine.%RST%
where python >nul 2>nul
if errorlevel 1 (
    echo(%WHT%  Install Python 3 first, then run: pip install platformio%RST%
    goto :fail
)
choice /c YN /n /m "  %ORG%Install PlatformIO now via pip? [Y/N]%RST% "
if errorlevel 2 goto :fail
echo.
python -m pip install --user platformio
python -m platformio --version >nul 2>nul
if errorlevel 1 (
    echo(%RED%  PlatformIO install failed. Try manually: pip install platformio%RST%
    goto :fail
)
set "PIO_CMD=python -m platformio"
echo(%GRN%  PlatformIO installed.%RST%

:havepio
echo.
echo(%GRY%  +---------------------------------------------------------------+%RST%
echo(%GRY%  ^|%RST%  %WHT%Build config%RST%                                                %GRY%^|%RST%
echo(%GRY%  ^|%RST%  env     : T_Embed_CC1101                                     %GRY%^|%RST%
echo(%GRY%  ^|%RST%  board   : T_Embed_PN532 ^(ESP32-S3^)                          %GRY%^|%RST%
echo(%GRY%  ^|%RST%  example : examples/factory                                 %GRY%^|%RST%
echo(%GRY%  +---------------------------------------------------------------+%RST%
echo(%GRY%  First build downloads the ESP32 toolchains - can take 5-15 min.%RST%
echo.

rem ------------------------------------------------------------
:build
echo(%ORG%  Compiling firmware...%RST%
echo.
%PIO_CMD% run -d "%REPO%" -e T_Embed_CC1101
if errorlevel 1 goto :buildfail
echo.
echo(%GRN%  Build OK.%RST%
echo(%GRY%  Image: %REPO%\.pio\build\T_Embed_CC1101\firmware.bin%RST%
echo.

choice /c YN /n /m "  %ORG%Flash it to the T-Embed now? [Y/N]%RST% "
if errorlevel 2 goto :bye
echo.
echo(%GRY%  Connect the T-Embed over USB...%RST%

rem ------------------------------------------------------------
:tryflash
set /a TRIES+=1
if %TRIES% GEQ 2 (
    echo(%ORG%  Retrying flash, attempt %TRIES% of 3...%RST%
    echo(%GRY%  If it keeps failing: hold BOOT, tap RESET, release BOOT,%RST%
    echo(%GRY%  then answer the port question below.%RST%
)
echo.
%PIO_CMD% run -d "%REPO%" -e T_Embed_CC1101 -t upload %UPLOAD_ARGS%
if not errorlevel 1 goto :flashed
if %TRIES% GEQ 3 goto :flashfail_final
call :listports
set "PORT="
set /p "PORT=  %ORG%Force a COM port ^(e.g. COM5^), blank = auto:%RST% "
if /i "%PORT%"=="" (set "UPLOAD_ARGS=") else (set "UPLOAD_ARGS=--upload-port %PORT%")
goto :tryflash

:flashfail_final
echo.
echo(%RED%  Flashing failed after %TRIES% attempts.%RST%
echo(%WHT%  Try: another USB cable, a different port, or hold BOOT while%RST%
echo(%WHT%  plugging the device in to force download mode.%RST%
goto :fail

rem ------------------------------------------------------------
:flashed
echo.
echo(%GRN%  Flash complete. Device updated - the dolphin salutes you.%RST%
call :dolphin_ok
echo.
choice /c YN /n /m "  %ORG%Open the serial monitor ^(115200 baud^)? [Y/N]%RST% "
if errorlevel 2 goto :bye
echo.
echo(%GRY%  Press Ctrl+C to leave the monitor.%RST%
echo.
%PIO_CMD% device monitor -d "%REPO%" -b 115200 --filter esp32_exception_decoder
goto :bye

rem ------------------------------------------------------------
:find_pio
where pio >nul 2>nul
if not errorlevel 1 (set "PIO_CMD=pio" & exit /b)
where python >nul 2>nul
if errorlevel 1 exit /b
python -m platformio --version >nul 2>nul
if not errorlevel 1 (set "PIO_CMD=python -m platformio" & exit /b)
exit /b

:listports
echo.
echo(%GRY%  Serial devices detected:%RST%
mode /status 2>nul | findstr /i "COM[0-9]"
if errorlevel 1 (
    echo(%GRY%  ^(none found by mode^) checking PlatformIO...%RST%
    %PIO_CMD% device list 2>nul
)
echo.
exit /b

:apply_theme
pushd "%REPO%" >nul 2>&1
git apply -R --check "..\flipper-style.patch" >nul 2>nul
if not errorlevel 1 (
    echo(%GRY%  Flipper theme already applied.%RST%
    popd
    exit /b
)
echo(%ORG%  Applying the Flipper Zero theme patch...%RST%
git apply --3way "..\flipper-style.patch" >nul 2>nul
if errorlevel 1 (
    echo(%GRY%  Theme patch skipped ^(could not apply cleanly^).%RST%
) else (
    echo(%GRN%  Flipper Zero theme applied.%RST%
)
popd
exit /b

:dolphin_ok
echo(%ORG%           _.--._%RST%
echo(%ORG%        .-'      '-.%RST%
echo(%ORG%       /   .-~~-.   \%RST%
echo(%ORG%      ^|   /      \   ^|%RST%
echo(%ORG%      ^|   \  \o/ ^|   ^|%RST%
echo(%ORG%       \   \__..-'   /%RST%
echo(%ORG%        '.    ^|    .'%RST%
echo(%ORG%          '-....-'%RST%
exit /b

:banner
echo(%ORG%           _.--._%RST%
echo(%ORG%        .-'      '-.%RST%
echo(%ORG%       /   .-~~-.   \      %WHT%T-EMBED CC1101%RST%
echo(%ORG%      ^|   /      \   ^|     %WHT%FIRMWARE BUILDER%RST%
echo(%ORG%      ^|   \  \o/ ^|   ^|     %GRY%Flipper Zero style edition%RST%
echo(%ORG%       \   \__..-'   /%RST%
echo(%ORG%        '.    ^|    .'%RST%
echo(%ORG%          '-....-'       %GRY%compile + flash, one script%RST%
echo.
goto :eof

:norepo
echo(%RED%  ERROR: %REPO% not found next to this script.%RST%
echo(%WHT%  Run update.bat first - it clones the firmware source for you.%RST%
goto :fail

:buildfail
echo.
echo(%RED%  ERROR: firmware build failed. Scroll up for compiler errors.%RST%
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
