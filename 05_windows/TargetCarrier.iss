#ifndef PayloadDir
  #error Provide PayloadDir with /DPayloadDir
#endif
#ifndef Bootstrapper
  #error Provide Bootstrapper with /DBootstrapper
#endif
#define ConsoleDir "runtime\programs\console"
#define PortableDir "runtime\TC-Portable"

[Setup]
AppId=TargetCarrier.Windows
AppName=TargetCarrier
AppVersion=7.0
DefaultDirName={localappdata}\Programs\TargetCarrier
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
DefaultGroupName=TargetCarrier
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\{#ConsoleDir}\TargetCarrier-7.0.exe
OutputBaseFilename=TargetCarrier-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Files]
Source: "{#PayloadDir}\console\*"; DestDir: "{app}\{#ConsoleDir}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#PayloadDir}\TC-Portable\*"; DestDir: "{app}\{#PortableDir}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Bootstrapper}"; DestName: "MicrosoftEdgeWebview2Setup.exe"; Flags: dontcopy

[Icons]
Name: "{autoprograms}\TargetCarrier\TargetCarrier 7.0"; Filename: "{app}\{#ConsoleDir}\TargetCarrier-7.0.exe"; WorkingDir: "{app}\{#ConsoleDir}"
Name: "{autoprograms}\TargetCarrier\TC-Portable"; Filename: "{app}\{#PortableDir}\TC-Portable.exe"; WorkingDir: "{app}\{#PortableDir}"

[Run]
Filename: "{app}\{#ConsoleDir}\TargetCarrier-7.0.exe"; WorkingDir: "{app}\{#ConsoleDir}"; Description: "Launch TargetCarrier 7.0"; Flags: nowait postinstall skipifsilent

[Code]
const
  WebView2Key = 'Software\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}';

function HasRuntimeVersion(Root: Integer): Boolean;
var
  Version: String;
begin
  Result := RegQueryStringValue(Root, WebView2Key, 'pv', Version) and
    (Version <> '') and (Version <> '0.0.0.0');
end;

function WebView2Installed(): Boolean;
begin
  Result := HasRuntimeVersion(HKLM32) or HasRuntimeVersion(HKCU32) or HasRuntimeVersion(HKCU64);
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ExitCode: Integer;
begin
  Result := '';
  if WebView2Installed() then
    Exit;

  Log('WebView2 Runtime is missing. Starting the Microsoft Evergreen Bootstrapper.');
  ExtractTemporaryFile('MicrosoftEdgeWebview2Setup.exe');
  if not Exec(ExpandConstant('{tmp}\MicrosoftEdgeWebview2Setup.exe'), '/silent /install',
    '', SW_HIDE, ewWaitUntilTerminated, ExitCode) then
    Result := Format('Microsoft WebView2 setup could not start (Windows error %d).', [ExitCode])
  else if not WebView2Installed() then begin
    if ExitCode = 3010 then
      NeedsRestart := True;
    Result := Format('Microsoft WebView2 Runtime is required. Check your network connection and retry setup (exit code %d).', [ExitCode]);
  end;
end;
