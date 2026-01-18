# install_git_hooks.ps1
# Installs the LOCKED tier commit-msg hook for GasAbilityGenerator
#
# Usage: Run once after cloning the repository
#   powershell -ExecutionPolicy Bypass -File "Tools/install_git_hooks.ps1"
#
# See: ClaudeContext/Handoffs/Architecture_Reference.md section 2.11

$ErrorActionPreference = "Stop"

Write-Host "Installing LOCKED tier git hooks..." -ForegroundColor Cyan

# Get repository root
$repoRoot = (git rev-parse --show-toplevel 2>$null)
if (-not $repoRoot) {
    Write-Host "ERROR: Not inside a git repository" -ForegroundColor Red
    exit 1
}
$repoRoot = $repoRoot.Trim()

# Hooks directory
$hooksDir = Join-Path $repoRoot ".git\hooks"
if (-not (Test-Path $hooksDir)) {
    Write-Host "ERROR: Hooks directory not found: $hooksDir" -ForegroundColor Red
    exit 1
}

# Path to our guard script (relative to repo root for portability)
$scriptRelPath = "Plugins/GasAbilityGenerator/Tools/locked_guard.py"
$scriptFullPath = Join-Path $repoRoot $scriptRelPath

if (-not (Test-Path $scriptFullPath)) {
    Write-Host "ERROR: Guard script not found: $scriptFullPath" -ForegroundColor Red
    exit 1
}

# Create commit-msg hook
$hookPath = Join-Path $hooksDir "commit-msg"

# Use 'py' launcher on Windows (handles Python version selection)
# Falls back to python3/python if py not available
$hookContent = @'
#!/bin/sh
# LOCKED tier guard hook - installed by install_git_hooks.ps1
# See: ClaudeContext/Handoffs/LOCKED_CONTRACTS.md

REPO_ROOT=$(git rev-parse --show-toplevel)
SCRIPT="$REPO_ROOT/Plugins/GasAbilityGenerator/Tools/locked_guard.py"

# Try py launcher first (Windows), then python3, then python
if command -v py >/dev/null 2>&1; then
    py "$SCRIPT" "$1"
elif command -v python3 >/dev/null 2>&1; then
    python3 "$SCRIPT" "$1"
else
    python "$SCRIPT" "$1"
fi
exit $?
'@

# Write hook file
Set-Content -Path $hookPath -Value $hookContent -NoNewline -Encoding ASCII

# Make executable (for Git Bash / WSL)
if (Test-Path $hookPath) {
    Write-Host "SUCCESS: Installed commit-msg hook" -ForegroundColor Green
    Write-Host "  Location: $hookPath" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Future commits touching LOCKED files will require:" -ForegroundColor Yellow
    Write-Host "  [LOCKED-CHANGE-APPROVED]" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "See: ClaudeContext/Handoffs/LOCKED_CONTRACTS.md" -ForegroundColor Gray
} else {
    Write-Host "ERROR: Failed to create hook file" -ForegroundColor Red
    exit 1
}
