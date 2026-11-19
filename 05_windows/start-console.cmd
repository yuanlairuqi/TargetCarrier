@echo off
setlocal
set "INSTALL_ROOT=%LOCALAPPDATA%\Programs\TargetCarrier"
for /f "tokens=2,*" %%A in ('reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Uninstall\TargetCarrier.Windows_is1" /v InstallLocation 2^>nul') do if /i "%%A"=="REG_SZ" set "INSTALL_ROOT=%%B"
set "APP=%INSTALL_ROOT%\runtime\programs\console\TargetCarrier-7.0.exe"
if exist "%APP%" (
  start "" "%APP%"
  exit /b
)
set "SETUP=%~dp0TargetCarrier-Setup.exe"
if exist "%SETUP%" (
  start "" "%SETUP%"
  exit /b
)
echo TargetCarrier 7.0 Setup is missing. Download TargetCarrier-Setup.exe from GitHub Releases.
pause
exit /b 1
