# GasAbilityGenerator Auto Build and Test Script
# This script automates the build-test-fix cycle for the plugin development

param(
    [switch]$SkipBuild,
    [switch]$SkipUE,
    [switch]$GenerateAssets,
    [string]$ManifestPath = "",
    [int]$MaxBuildRetries = 3
)

# Configuration
$ProjectRoot = "C:\Unreal Projects\NP22B57"
$ProjectFile = "$ProjectRoot\NP22B57.uproject"
$SolutionFile = "$ProjectRoot\NP22B57.sln"
$PluginRoot = "$ProjectRoot\Plugins\GasAbilityGenerator"
$UEPath = "C:\Program Files\Epic Games\UE_5.7"
$UBTPath = "$UEPath\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
$UEEditorPath = "$UEPath\Engine\Binaries\Win64\UnrealEditor.exe"
$LogDir = "$PluginRoot\Tools\Logs"
$ErrorLogFile = "$LogDir\build_errors.log"
$GenerationLogFile = "$LogDir\generation.log"

# Create log directory
if (-not (Test-Path $LogDir)) {
    New-Item -ItemType Directory -Path $LogDir -Force | Out-Null
}

# Timestamp for logs
$Timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"

function Write-Log {
    param([string]$Message, [string]$Level = "INFO")
    $LogMessage = "[$Timestamp] [$Level] $Message"
    Write-Host $LogMessage -ForegroundColor $(switch($Level) {
        "ERROR" { "Red" }
        "WARN" { "Yellow" }
        "SUCCESS" { "Green" }
        default { "White" }
    })
    Add-Content -Path "$LogDir\automation_$Timestamp.log" -Value $LogMessage
}

function Build-Plugin {
    Write-Log "Starting UBT build..." "INFO"

    $BuildOutput = & "$UBTPath" NP22B57Editor Win64 Development "-Project=$ProjectFile" 2>&1
    $BuildExitCode = $LASTEXITCODE

    # Save build output
    $BuildOutput | Out-File "$LogDir\ubt_build_$Timestamp.log" -Encoding UTF8

    if ($BuildExitCode -ne 0) {
        Write-Log "UBT Build FAILED with exit code $BuildExitCode" "ERROR"

        # Extract error lines
        $Errors = $BuildOutput | Select-String -Pattern "error C\d+:|error:|fatal error" | ForEach-Object { $_.Line }
        if ($Errors) {
            Write-Log "Build Errors:" "ERROR"
            $Errors | ForEach-Object { Write-Log "  $_" "ERROR" }
            $Errors | Out-File $ErrorLogFile -Encoding UTF8
        }
        return $false
    }

    Write-Log "UBT Build SUCCEEDED" "SUCCESS"
    return $true
}

function Build-Solution {
    Write-Log "Starting MSBuild..." "INFO"

    # Find MSBuild
    $MSBuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | Select-Object -First 1

    if (-not $MSBuild) {
        Write-Log "MSBuild not found!" "ERROR"
        return $false
    }

    $BuildOutput = & "$MSBuild" "$SolutionFile" /t:Build /p:Configuration="Development Editor" /p:Platform=Win64 /verbosity:minimal 2>&1
    $BuildExitCode = $LASTEXITCODE

    $BuildOutput | Out-File "$LogDir\msbuild_$Timestamp.log" -Encoding UTF8

    if ($BuildExitCode -ne 0) {
        Write-Log "MSBuild FAILED" "ERROR"
        $Errors = $BuildOutput | Select-String -Pattern "error C\d+:|error CS\d+:|error:" | ForEach-Object { $_.Line }
        if ($Errors) {
            $Errors | ForEach-Object { Write-Log "  $_" "ERROR" }
            $Errors | Out-File $ErrorLogFile -Append -Encoding UTF8
        }
        return $false
    }

    Write-Log "MSBuild SUCCEEDED" "SUCCESS"
    return $true
}

function Start-UnrealEditor {
    param([switch]$RunCommandlet)

    Write-Log "Starting Unreal Editor..." "INFO"

    $UELogFile = "$LogDir\UE_$Timestamp.log"

    if ($RunCommandlet -and $ManifestPath) {
        # Run with commandlet for automated asset generation
        $Args = @(
            "`"$ProjectFile`"",
            "-run=GasAbilityGenerator",
            "-manifest=`"$ManifestPath`"",
            "-log=`"$UELogFile`"",
            "-unattended",
            "-nosplash"
        )
        Write-Log "Running with commandlet: $Args" "INFO"
        $Process = Start-Process -FilePath $UEEditorPath -ArgumentList $Args -PassThru -Wait
    }
    else {
        # Normal editor launch
        $Process = Start-Process -FilePath $UEEditorPath -ArgumentList "`"$ProjectFile`"" -PassThru
        Write-Log "Unreal Editor launched (PID: $($Process.Id))" "INFO"
        return $Process
    }

    return $null
}

function Get-UELogErrors {
    param([string]$LogPath)

    if (-not (Test-Path $LogPath)) {
        # Try to find the latest UE log
        $LogPath = "$env:LOCALAPPDATA\UnrealEngine\5.7\Saved\Logs\NP22B57.log"
    }

    if (Test-Path $LogPath) {
        Write-Log "Reading UE log: $LogPath" "INFO"
        $LogContent = Get-Content $LogPath -Tail 500

        # Extract errors and warnings related to generation
        $GenerationLogs = $LogContent | Select-String -Pattern "GasAbilityGenerator|EventGraph|FAILED|ERROR|WARNING.*connection|WARNING.*pin" | ForEach-Object { $_.Line }

        if ($GenerationLogs) {
            $GenerationLogs | Out-File $GenerationLogFile -Encoding UTF8
            Write-Log "Found $($GenerationLogs.Count) relevant log entries" "INFO"
            return $GenerationLogs
        }
    }

    Write-Log "No UE log found or no relevant entries" "WARN"
    return @()
}

function Export-ErrorSummary {
    Write-Log "Generating error summary..." "INFO"

    $Summary = @"
# GasAbilityGenerator Build/Test Summary
Generated: $(Get-Date)

## Build Status
"@

    if (Test-Path $ErrorLogFile) {
        $Summary += "`n### Build Errors:`n"
        $Summary += Get-Content $ErrorLogFile | ForEach-Object { "- $_" }
    }

    if (Test-Path $GenerationLogFile) {
        $Summary += "`n`n### Generation Logs:`n"
        $Summary += Get-Content $GenerationLogFile | ForEach-Object { "- $_" }
    }

    $SummaryFile = "$LogDir\summary_$Timestamp.md"
    $Summary | Out-File $SummaryFile -Encoding UTF8
    Write-Log "Summary saved to: $SummaryFile" "SUCCESS"

    return $SummaryFile
}

# Main automation loop
function Start-AutomationCycle {
    Write-Log "========================================" "INFO"
    Write-Log "Starting GasAbilityGenerator Automation" "INFO"
    Write-Log "========================================" "INFO"

    $BuildAttempt = 0
    $BuildSuccess = $false

    # Step 1: Build Plugin with UBT
    if (-not $SkipBuild) {
        while (-not $BuildSuccess -and $BuildAttempt -lt $MaxBuildRetries) {
            $BuildAttempt++
            Write-Log "Build attempt $BuildAttempt of $MaxBuildRetries" "INFO"

            $BuildSuccess = Build-Plugin

            if (-not $BuildSuccess) {
                Write-Log "Build failed. Check $ErrorLogFile for errors." "ERROR"
                Write-Log "Fix errors and press Enter to retry, or Ctrl+C to abort..." "WARN"
                Read-Host
            }
        }

        if (-not $BuildSuccess) {
            Write-Log "Max build retries reached. Aborting." "ERROR"
            Export-ErrorSummary
            return $false
        }
    }

    # Step 2: Launch Unreal Editor
    if (-not $SkipUE) {
        $UEProcess = Start-UnrealEditor

        if ($UEProcess) {
            Write-Log "Unreal Editor is running. Perform your tests." "INFO"
            Write-Log "Press Enter when done to collect logs and continue..." "INFO"
            Read-Host

            # Collect logs
            $Logs = Get-UELogErrors

            if ($Logs.Count -gt 0) {
                Write-Log "Found $($Logs.Count) log entries to analyze" "INFO"
            }
        }
    }

    # Step 3: Generate summary
    $SummaryFile = Export-ErrorSummary

    Write-Log "========================================" "INFO"
    Write-Log "Automation cycle complete" "SUCCESS"
    Write-Log "Summary: $SummaryFile" "INFO"
    Write-Log "========================================" "INFO"

    return $true
}

# Run the automation
Start-AutomationCycle
