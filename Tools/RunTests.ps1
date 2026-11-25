<#
.SYNOPSIS
    Builds the editor and runs the QuidProWiz automation tests headlessly.

.EXAMPLE
    .\Tools\RunTests.ps1                    # build, then run every QuidProWiz test
    .\Tools\RunTests.ps1 -Filter QuidProWiz.Quaffle -SkipBuild

    Exit code is 0 when every test passes. The JSON/HTML report is written to
    Saved\Automation\Reports and the full log to Saved\Automation\RunTests.log.
    Close the editor first (Live Coding blocks command-line builds).
#>
param(
    [string]$Filter = "QuidProWiz",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $ProjectRoot "QuidProWiz.uproject"
$EngineVersion = (Get-Content $Project -Raw | ConvertFrom-Json).EngineAssociation

$EngineRoot = $null
try {
    $EngineRoot = (Get-ItemProperty "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$EngineVersion" -ErrorAction Stop).InstalledDirectory
} catch {}
if (-not $EngineRoot) { $EngineRoot = "C:\Program Files\Epic Games\UE_$EngineVersion" }

$BuildBat = Join-Path $EngineRoot "Engine\Build\BatchFiles\Build.bat"
$EditorCmd = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
if (-not (Test-Path $EditorCmd)) { throw "Unreal Engine $EngineVersion not found at $EngineRoot" }

if (-not $SkipBuild) {
    Write-Host "Building QuidProWizEditor..."
    & $BuildBat QuidProWizEditor Win64 Development "-Project=$Project" -WaitMutex
    if ($LASTEXITCODE -ne 0) { throw "Build failed (exit $LASTEXITCODE)" }
}

$AutomationDir = Join-Path $ProjectRoot "Saved\Automation"
$ReportDir = Join-Path $AutomationDir "Reports"
$Log = Join-Path $AutomationDir "RunTests.log"
New-Item -ItemType Directory -Force $ReportDir | Out-Null

Write-Host "Running automation tests matching '$Filter'..."
& $EditorCmd $Project "-ExecCmds=Automation RunTests $Filter; Quit" `
    "-TestExit=Automation Test Queue Empty" "-ReportExportPath=$ReportDir" `
    -Unattended -NullRHI -NoSound -NoSplash -NoP4 "-abslog=$Log" | Out-Null

$Results = Select-String -Path $Log -Pattern "Test Completed\. Result=\{(\w+)\} Name=\{([^}]*)\} Path=\{([^}]*)\}"
if (-not $Results) { throw "No test results found; see $Log" }

$Failed = 0
foreach ($Match in $Results) {
    $Outcome = $Match.Matches[0].Groups[1].Value
    $Path = $Match.Matches[0].Groups[3].Value
    if ($Outcome -eq "Success") {
        Write-Host "  PASS  $Path" -ForegroundColor Green
    } else {
        Write-Host "  $($Outcome.ToUpper())  $Path" -ForegroundColor Red
        $Failed++
    }
}

Write-Host ""
Write-Host "$($Results.Count - $Failed) passed, $Failed failed. Report: $ReportDir"
exit [int]($Failed -gt 0)
