param(
    [Parameter(Mandatory)][string]$ConsoleBuild,
    [Parameter(Mandatory)][string]$Iscc,
    [Parameter(Mandatory)][string]$WebView2Bootstrapper
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$console = (Resolve-Path -LiteralPath $ConsoleBuild).Path
$compiler = (Resolve-Path -LiteralPath $Iscc).Path
$bootstrapper = (Resolve-Path -LiteralPath $WebView2Bootstrapper).Path
$portable = Join-Path $repo '05_上位机\项目资料\TC-Portable'

if (!(Test-Path -LiteralPath (Join-Path $console 'TargetCarrier-7.0.exe') -PathType Leaf) -or
    !(Test-Path -LiteralPath (Join-Path $console '_internal\web\index.html') -PathType Leaf)) {
    throw 'Use the complete TargetCarrier-7.0 onedir output from build-unified.ps1.'
}
if (!(Test-Path -LiteralPath (Join-Path $portable 'TC-Portable.exe') -PathType Leaf) -or
    !(Test-Path -LiteralPath (Join-Path $portable '_internal') -PathType Container)) {
    throw 'The repository must include the complete TC-Portable runtime.'
}
$signature = Get-AuthenticodeSignature -LiteralPath $bootstrapper
if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'O=Microsoft Corporation(?:,|$)') {
    throw 'Use a valid Microsoft-signed Evergreen WebView2 Bootstrapper.'
}

$payload = Join-Path ([System.IO.Path]::GetTempPath()) ('tc-setup-' + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Path $payload | Out-Null
try {
    foreach ($runtime in @(
        @{ Source = $console; Folder = 'console'; Exe = 'TargetCarrier-7.0.exe' },
        @{ Source = $portable; Folder = 'TC-Portable'; Exe = 'TC-Portable.exe' }
    )) {
        $destination = Join-Path $payload $runtime.Folder
        New-Item -ItemType Directory -Path $destination | Out-Null
        Copy-Item -LiteralPath (Join-Path $runtime.Source $runtime.Exe) -Destination $destination
        Copy-Item -LiteralPath (Join-Path $runtime.Source '_internal') -Destination $destination -Recurse
    }
    & $compiler "/DPayloadDir=$payload" "/DBootstrapper=$bootstrapper" "/O$PSScriptRoot" (Join-Path $PSScriptRoot 'TargetCarrier.iss')
    if ($LASTEXITCODE) { throw "Inno Setup compiler exited with code $LASTEXITCODE." }
    Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'TargetCarrier-Setup.exe') -Algorithm SHA256
} finally {
    $tempRoot = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    $resolvedPayload = (Resolve-Path -LiteralPath $payload).Path
    if ($resolvedPayload.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase) -and
        (Split-Path -Leaf $resolvedPayload) -match '^tc-setup-[a-f0-9]{8}$') {
        Remove-Item -LiteralPath $resolvedPayload -Recurse -Force
    }
}
