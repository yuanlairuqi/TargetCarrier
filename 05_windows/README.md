# TargetCarrier 7.0 Windows 安装包

安装包包含重新构建的 7.0 控制台、完整的 PyInstaller 运行目录和 TC-Portable。采用当前用户安装，默认目录为 `%LOCALAPPDATA%\Programs\TargetCarrier`，并为两款程序创建开始菜单入口。

## 构建

1. 在 `05_windows/project/source/current` 运行 `build-unified.ps1`。使用 x64 Python 3.11，并按仓库的锁定依赖构建 `dist/TargetCarrier-7.0`。
2. 准备 Inno Setup 7 的 `ISCC.exe` 和微软签名的 [WebView2 Evergreen Bootstrapper](https://developer.microsoft.com/microsoft-edge/webview2/#download-section)。
3. 在本目录执行以下命令，把路径换成指定产物和工具的路径。

```powershell
.\build-installer.ps1 -ConsoleBuild 'C:\build\dist\TargetCarrier-7.0' -Iscc 'C:\tools\Inno Setup 7\ISCC.exe' -WebView2Bootstrapper 'C:\tools\MicrosoftEdgeWebview2Setup.exe'
```

脚本从仓库读取 `TC-Portable` 的 EXE 和依赖，输出 `05_windows/TargetCarrier-Setup.exe` 和控制台中的 SHA-256 校验值。安装器检查当前用户和系统已安装的 WebView2。机器缺少运行时才会启动微软的联网安装程序。
