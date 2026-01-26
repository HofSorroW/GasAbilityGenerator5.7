# Claude Code Integration Automation Script
# This script runs the build-test cycle and outputs structured data for Claude to parse

param(
    [ValidateSet("build", "run", "logs", "full", "generate", "cycle")]
    [string]$Action = "build"
)

$ErrorActionPreference = "Continue"

# Paths
$ProjectRoot = "C:\Unreal Projects\NP22B57"
$ProjectFile = "$ProjectRoot\NP22B57.uproject"
$UERoot = "C:\Program Files\Epic Games\UE_5.7"
$UBT = "$UERoot\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
$Editor = "$UERoot\Engine\Binaries\Win64\UnrealEditor.exe"
$EditorCmd = "$UERoot\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$PluginRoot = "$ProjectRoot\Plugins\GasAbilityGenerator"
$LogDir = "$PluginRoot\Tools\Logs"
$UELogPath = "$env:LOCALAPPDATA\UnrealEngine\5.7\Saved\Logs\NP22B57.log"

# Ensure log directory exists
New-Item -ItemType Directory -Path $LogDir -Force | Out-Null

function Write-StructuredOutput {
    param(
        [string]$Type,
        [string]$Status,
        [string]$Message,
        [array]$Details = @()
    )

    Write-Host "[$Type] $Status : $Message"
    if ($Details.Count -gt 0) {
        $Details | ForEach-Object { Write-Host "  - $_" }
    }
}

function Invoke-UBTBuild {
    Write-Host "=== UBT BUILD START ==="

    $output = & $UBT NP22B57Editor Win64 Development "-Project=$ProjectFile" 2>&1
    $exitCode = $LASTEXITCODE

    # Save full output
    $output | Out-File "$LogDir\ubt_latest.log" -Encoding UTF8

    if ($exitCode -eq 0) {
        Write-StructuredOutput -Type "BUILD" -Status "SUCCESS" -Message "UBT build completed successfully"
        return $true
    }
    else {
        # Extract errors
        $errors = $output | Where-Object { $_ -match "error C\d+:|error:|fatal error" }

        Write-StructuredOutput -Type "BUILD" -Status "FAILED" -Message "UBT build failed with $($errors.Count) error(s)" -Details $errors

        Write-Host ""
        Write-Host "=== ERRORS FOR CLAUDE TO FIX ==="
        $errors | ForEach-Object {
            # Parse error format: file(line): error CODE: message
            if ($_ -match "(.+)\((\d+)\):\s*error\s+(\w+):\s*(.+)") {
                Write-Host "FILE: $($Matches[1])"
                Write-Host "LINE: $($Matches[2])"
                Write-Host "CODE: $($Matches[3])"
                Write-Host "MSG:  $($Matches[4])"
                Write-Host "---"
            }
            else {
                Write-Host $_
            }
        }
        Write-Host "=== END ERRORS ==="

        return $false
    }
}

function Start-Editor {
    Write-Host "=== LAUNCHING UNREAL EDITOR ==="
    Start-Process -FilePath $Editor -ArgumentList "`"$ProjectFile`""
    Write-StructuredOutput -Type "EDITOR" -Status "LAUNCHED" -Message "Unreal Editor started"
}

function Get-GenerationLogs {
    Write-Host "=== GENERATION LOGS ==="

    if (-not (Test-Path $UELogPath)) {
        Write-StructuredOutput -Type "LOGS" -Status "NOT_FOUND" -Message "UE log file not found at $UELogPath"
        return
    }

    # Read last 1000 lines
    $logContent = Get-Content $UELogPath -Tail 1000

    # Filter for GasAbilityGenerator related entries
    $generationLogs = $logContent | Where-Object {
        $_ -match "GasAbilityGenerator|EventGraph|LogGeneration|FAILED|error.*connection|WARN.*pin|Created node|Connected:"
    }

    if ($generationLogs.Count -eq 0) {
        Write-StructuredOutput -Type "LOGS" -Status "EMPTY" -Message "No generation-related logs found"
        return
    }

    Write-Host "Found $($generationLogs.Count) relevant log entries:"
    Write-Host ""

    # Categorize logs
    $errors = $generationLogs | Where-Object { $_ -match "FAILED|ERROR" }
    $warnings = $generationLogs | Where-Object { $_ -match "WARN|WARNING" }
    $info = $generationLogs | Where-Object { $_ -match "Created|Connected|Generated" }

    if ($errors.Count -gt 0) {
        Write-Host "=== ERRORS ($($errors.Count)) ==="
        $errors | ForEach-Object { Write-Host $_ }
        Write-Host ""
    }

    if ($warnings.Count -gt 0) {
        Write-Host "=== WARNINGS ($($warnings.Count)) ==="
        $warnings | ForEach-Object { Write-Host $_ }
        Write-Host ""
    }

    Write-Host "=== SUMMARY ==="
    Write-Host "Errors: $($errors.Count)"
    Write-Host "Warnings: $($warnings.Count)"
    Write-Host "Info entries: $($info.Count)"

    # Save to file for Claude to read
    $generationLogs | Out-File "$LogDir\generation_latest.log" -Encoding UTF8
    Write-Host ""
    Write-Host "Full logs saved to: $LogDir\generation_latest.log"
}

function Invoke-FullCycle {
    Write-Host "============================================"
    Write-Host "FULL AUTOMATION CYCLE"
    Write-Host "============================================"
    Write-Host ""

    # Step 1: Build
    $buildSuccess = Invoke-UBTBuild

    if (-not $buildSuccess) {
        Write-Host ""
        Write-Host "[CYCLE] Build failed. Fix errors above and run again."
        return $false
    }

    # Step 2: Launch editor
    Start-Editor

    Write-Host ""
    Write-Host "[CYCLE] Editor launched. After generating assets:"
    Write-Host "  1. Use the GasAbilityGenerator plugin to generate assets"
    Write-Host "  2. Close the editor"
    Write-Host "  3. Run: .\claude_automation.ps1 -Action logs"
    Write-Host ""

    return $true
}

# Main execution
switch ($Action) {
    "build" {
        Invoke-UBTBuild | Out-Null
    }
    "run" {
        Start-Editor
    }
    "logs" {
        Get-GenerationLogs
    }
    "full" {
        Invoke-FullCycle | Out-Null
    }
    "generate" {
        Write-Host "=== RUNNING COMMANDLET GENERATION ==="

        # Find manifest file
        $ManifestPath = "$PluginRoot\ClaudeContext\manifest.yaml"
        if (-not (Test-Path $ManifestPath)) {
            Write-Host "[ERROR] Manifest not found at: $ManifestPath"
            return
        }

        $OutputLog = "$LogDir\commandlet_output.log"
        # Convert to forward slashes for UE compatibility and quote properly
        $ManifestPathUE = $ManifestPath -replace '\\', '/'
        $OutputLogUE = $OutputLog -replace '\\', '/'

        Write-Host "Manifest: $ManifestPathUE"
        Write-Host "Output: $OutputLogUE"
        Write-Host ""

        # Build argument list with proper quoting
        # v4.39.3: Removed -nullrhi - it prevents Blueprint assets from persisting to disk
        $args = @(
            "`"$ProjectFile`"",
            "-run=GasAbilityGenerator",
            "-manifest=`"$ManifestPathUE`"",
            "-output=`"$OutputLogUE`"",
            "-unattended",
            "-nosplash"
        )

        Write-Host "Running: $EditorCmd $($args -join ' ')"

        # Run commandlet and capture output (use EditorCmd for headless operation)
        $process = Start-Process -FilePath $EditorCmd -ArgumentList $args -NoNewWindow -Wait -PassThru -RedirectStandardOutput "$LogDir\commandlet_stdout.log" -RedirectStandardError "$LogDir\commandlet_stderr.log"
        $exitCode = $process.ExitCode

        # Display stdout if captured
        if (Test-Path "$LogDir\commandlet_stdout.log") {
            $stdout = Get-Content "$LogDir\commandlet_stdout.log" -Raw
            if ($stdout) { Write-Host $stdout }
        }

        if (Test-Path $OutputLog) {
            Write-Host ""
            Write-Host "=== GENERATION LOG ==="
            Get-Content $OutputLog
        } else {
            Write-Host "[WARN] Output log not created at: $OutputLog"
            # Fallback: extract from UE log
            if (Test-Path $UELogPath) {
                Write-Host "Extracting from UE log..."
                $ueContent = Get-Content $UELogPath -Tail 500 | Where-Object { $_ -match "GasAbilityGenerator" }
                $ueContent | Out-File $OutputLog -Encoding UTF8
                Get-Content $OutputLog
            }
        }

        if ($exitCode -eq 0) {
            Write-StructuredOutput -Type "GENERATE" -Status "SUCCESS" -Message "Asset generation completed"
        } else {
            Write-StructuredOutput -Type "GENERATE" -Status "FAILED" -Message "Asset generation failed with code $exitCode"
        }
    }
    "cycle" {
        Write-Host "=== FULL AUTOMATED CYCLE ==="
        Write-Host ""

        # Step 1: Build
        $buildResult = Invoke-UBTBuild
        if (-not $buildResult) {
            Write-Host "[CYCLE STOPPED] Build failed. Fix errors and run again."
            return
        }

        # Step 2: Run commandlet generation
        Write-Host ""
        $ManifestPath = "$PluginRoot\ClaudeContext\manifest.yaml"
        if (-not (Test-Path $ManifestPath)) {
            Write-Host "[ERROR] Manifest not found at: $ManifestPath"
            return
        }

        $OutputLog = "$LogDir\commandlet_output.log"
        # Convert to forward slashes for UE compatibility
        $ManifestPathUE = $ManifestPath -replace '\\', '/'
        $OutputLogUE = $OutputLog -replace '\\', '/'

        Write-Host "Running asset generation commandlet..."
        Write-Host "Manifest: $ManifestPathUE"
        Write-Host "Output: $OutputLogUE"

        # v4.39.3: Removed -nullrhi - it prevents Blueprint assets from persisting to disk
        $args = @(
            "`"$ProjectFile`"",
            "-run=GasAbilityGenerator",
            "-manifest=`"$ManifestPathUE`"",
            "-output=`"$OutputLogUE`"",
            "-unattended",
            "-nosplash"
        )

        # Run commandlet and capture output (use EditorCmd for headless operation)
        $process = Start-Process -FilePath $EditorCmd -ArgumentList $args -NoNewWindow -Wait -PassThru -RedirectStandardOutput "$LogDir\commandlet_stdout.log" -RedirectStandardError "$LogDir\commandlet_stderr.log"
        $exitCode = $process.ExitCode

        # Save stdout to full log
        if (Test-Path "$LogDir\commandlet_stdout.log") {
            Copy-Item "$LogDir\commandlet_stdout.log" "$LogDir\commandlet_full.log" -Force
        }

        # Display relevant output
        if (Test-Path $OutputLog) {
            Write-Host ""
            Write-Host "=== GENERATION RESULTS ==="
            Get-Content $OutputLog | Where-Object { $_ -match "\[NEW\]|\[FAIL\]|ERROR|Summary|---" }
        } else {
            Write-Host "[WARN] Output log not created at: $OutputLog"
            # Fallback: extract from UE log
            if (Test-Path $UELogPath) {
                Write-Host "Extracting from UE log..."
                $ueContent = Get-Content $UELogPath -Tail 500 | Where-Object { $_ -match "GasAbilityGenerator" }
                $ueContent | Out-File $OutputLog -Encoding UTF8
                if (Test-Path $OutputLog) {
                    Get-Content $OutputLog | Where-Object { $_ -match "\[NEW\]|\[FAIL\]|ERROR|Summary|---" }
                }
            }
        }

        # Step 3: Show any errors
        Write-Host ""
        Write-Host "=== CYCLE COMPLETE ==="

        # Check for failures
        if (Test-Path $OutputLog) {
            $failures = Get-Content $OutputLog | Where-Object { $_ -match "FAIL|ERROR" }
            if ($failures) {
                Write-Host "[ISSUES FOUND]"
                $failures | ForEach-Object { Write-Host "  $_" }
            } else {
                Write-Host "[SUCCESS] No generation errors detected"
            }
        }
    }
}
