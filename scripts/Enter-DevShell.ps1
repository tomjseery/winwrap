#Requires -Version 5.1
[CmdletBinding()]
param(
    [string]$VsInstallPath,
    [string[]]$Component = @()
)

$ErrorActionPreference = 'Stop'
if (!$VsInstallPath) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (!(Test-Path -LiteralPath $vswhere)) { throw 'Install the Microsoft C++ build tools first.' }
    $required = @('Microsoft.VisualStudio.Component.VC.Tools.x86.x64') + $Component
    $VsInstallPath = & $vswhere -latest -products '*' -requires $required -property installationPath
    if ($LASTEXITCODE -ne 0 -or !$VsInstallPath) { throw "No installed MSVC x64/x86 toolchain with: $($required -join ', ')." }
}
$launcher = Join-Path $VsInstallPath 'Common7/Tools/Launch-VsDevShell.ps1'
if (!(Test-Path -LiteralPath $launcher)) { throw "Developer PowerShell launcher missing: $launcher" }
& $launcher -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
if (!$env:VCToolsInstallDir) { throw 'Developer PowerShell did not initialize MSVC.' }
Get-Command cl -ErrorAction Stop | Out-Null

# CMake built for MSYS2/MinGW/Cygwin drives rc.exe through a wrapper that fails on MSVC
# resource files, so a native Windows CMake must come first on PATH.
$bundledCMake = Join-Path $VsInstallPath 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin'
$candidates = @(if (Test-Path -LiteralPath (Join-Path $bundledCMake 'cmake.exe')) { Join-Path $bundledCMake 'cmake.exe' }) +
    @(Get-Command cmake -All -CommandType Application -ErrorAction SilentlyContinue | ForEach-Object Source)
$cmake = $candidates | Where-Object { $_ -notmatch '[\\/](msys64|msys2|mingw(32|64)?|cygwin(64)?)[\\/]' } | Select-Object -First 1
if (!$cmake) { throw 'No native Windows CMake found; install the Visual Studio CMake component or CMake for Windows.' }
$env:PATH = (Split-Path -Parent $cmake) + [IO.Path]::PathSeparator + $env:PATH
