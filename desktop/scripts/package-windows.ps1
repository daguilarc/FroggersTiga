$ErrorActionPreference = 'Stop'

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$DesktopDir = (Resolve-Path (Join-Path $ScriptDir '..')).Path
$BuildDir = Join-Path $DesktopDir 'build'
$ArtefactsDir = Join-Path $BuildDir 'FroggersTigaDesktop_artefacts'
$ReleaseDir = Join-Path $ArtefactsDir 'Release'
$DistDir = Join-Path $DesktopDir 'dist'
$IssFile = Join-Path $DesktopDir 'installer' 'FroggersTiga.iss'

$Version = & (Join-Path $ScriptDir 'read-version.ps1')

if (Test-Path (Join-Path $ReleaseDir 'FroggersTiga.exe')) {
    $SourceDir = $ReleaseDir
} elseif (Test-Path (Join-Path $ArtefactsDir 'FroggersTiga.exe')) {
    $SourceDir = $ArtefactsDir
} else {
    Write-Error @"
Release build not found.
  expected: $ReleaseDir\FroggersTiga.exe
  fallback: $ArtefactsDir\FroggersTiga.exe
Run: cd desktop; cmake -B build; cmake --build build --config Release
"@
}

$IsccCandidates = @(
    "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
    "$env:ProgramFiles\Inno Setup 6\ISCC.exe"
)

$Iscc = $IsccCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $Iscc) {
    Write-Error @"
Inno Setup 6 (ISCC.exe) not found.
Install from: https://jrsoftware.org/isinfo.php
Or on CI: choco install innosetup
"@
}

New-Item -ItemType Directory -Force -Path $DistDir | Out-Null

& $Iscc `
    "/DMyAppVersion=$Version" `
    "/DReleaseDir=$SourceDir" `
    "/DOutputDir=$DistDir" `
    $IssFile

$OutputExe = Join-Path $DistDir "FroggersTiga-$Version-Windows-Setup.exe"
if (-not (Test-Path $OutputExe)) {
    Write-Error "expected installer not created: $OutputExe"
}

Write-Output "created: $OutputExe"
