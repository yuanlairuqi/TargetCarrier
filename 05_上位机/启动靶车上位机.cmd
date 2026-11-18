@echo off
chcp 65001 >nul
pushd "%~dp0项目资料\程序\控制台7.0"
if not exist "TargetCarrier-7.0.exe" (
  echo The Windows package must be fully extracted.
  pause
  exit /b 1
)
start "" "TargetCarrier-7.0.exe"
popd
