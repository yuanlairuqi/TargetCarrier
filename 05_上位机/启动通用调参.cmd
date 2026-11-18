@echo off
chcp 65001 >nul
pushd "%~dp0项目资料\TC-Portable"
if not exist "TC-Portable.exe" (
  echo The Windows package must be fully extracted.
  pause
  exit /b 1
)
start "" "TC-Portable.exe"
popd
