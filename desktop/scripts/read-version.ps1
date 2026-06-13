$ErrorActionPreference = 'Stop'

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$CmakeFile = Join-Path $ScriptDir '..' 'CMakeLists.txt' | Resolve-Path

$content = Get-Content -Path $CmakeFile -Raw
if ($content -match 'project\(FroggersTigaDesktop VERSION ([0-9]+\.[0-9]+\.[0-9]+)') {
    Write-Output $Matches[1]
    exit 0
}

Write-Error "could not parse VERSION from $CmakeFile"
exit 1
