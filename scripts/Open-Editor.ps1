#Requires -Version 5.1
[CmdletBinding()]
param(
    [string]$VsInstallPath,
    [string]$Editor = 'code'
)

$ErrorActionPreference = 'Stop'
# clangd finds the MSVC headers and Windows SDK only through the developer environment: its own
# discovery picks the newest Visual Studio Setup instance even when that instance has no C++ tools.
& (Join-Path $PSScriptRoot 'Enter-DevShell.ps1') -VsInstallPath $VsInstallPath
$editorCommand = Get-Command $Editor -CommandType Application, ExternalScript -ErrorAction Stop | Select-Object -First 1
& $editorCommand.Source (Split-Path -Parent $PSScriptRoot)
if ($LASTEXITCODE) { throw "$Editor exited with $LASTEXITCODE." }
