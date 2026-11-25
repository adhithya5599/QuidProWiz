<#
.SYNOPSIS
    Builds a Blender asset headlessly from its script in Tools\Blender\assets.

.EXAMPLE
    .\Tools\Blender\BuildAsset.ps1 -Asset SM_PipelineTest

    Writes SourceArt\Generated\<Asset>\<Asset>.fbx, .blend and _preview.png.
    Import the FBX into Unreal from the Content Browser (or let Claude do it via Unreal MCP).
#>
param(
    [Parameter(Mandatory = $true)][string]$Asset,
    [string]$BlenderExe
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Script = Join-Path $PSScriptRoot "assets\$Asset.py"
if (-not (Test-Path $Script)) { throw "No asset script at $Script" }

if (-not $BlenderExe) {
    $Candidates = Get-ChildItem "C:\Program Files\Blender Foundation" -Directory -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending | ForEach-Object { Join-Path $_.FullName "blender.exe" } |
        Where-Object { Test-Path $_ }
    $BlenderExe = $Candidates | Select-Object -First 1
}
if (-not $BlenderExe -or -not (Test-Path $BlenderExe)) { throw "Blender not found; pass -BlenderExe" }

$OutRoot = Join-Path $ProjectRoot "SourceArt\Generated"

Write-Host "Building $Asset with $BlenderExe"
# Blender logs to stderr; don't let PowerShell treat that as a terminating error.
$ErrorActionPreference = "Continue"
$Output = & $BlenderExe --background --factory-startup --python-exit-code 1 --python $Script -- --out $OutRoot 2>&1 |
    ForEach-Object { "$_" }
$ExitCode = $LASTEXITCODE
$ErrorActionPreference = "Stop"

$Output | Where-Object { $_ -match "^QPW_|Error|Traceback|line \d+" } | ForEach-Object { Write-Host $_ }
if ($ExitCode -ne 0) { throw "Blender failed (exit $ExitCode)" }
