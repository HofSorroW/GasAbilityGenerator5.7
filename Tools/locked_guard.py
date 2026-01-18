#!/usr/bin/env python3
"""
Git commit-msg hook guard for LOCKED tier files.

Blocks commits that modify locked files unless the commit message contains
the bypass token [LOCKED-CHANGE-APPROVED].

Usage: Called by git commit-msg hook with commit message file path as argument.
Install: Run Tools/install_git_hooks.ps1 once after cloning.

See: ClaudeContext/Handoffs/LOCKED_CONTRACTS.md for invariant definitions
     ClaudeContext/Handoffs/LOCKED_SYSTEMS.md for file mappings
"""

import subprocess
import sys
import fnmatch

BYPASS_TOKEN = "[LOCKED-CHANGE-APPROVED]"

# Locked files - require explicit approval to modify
# Paths relative to repository root
LOCKED_GLOBS = [
    # Contract documentation
    "ClaudeContext/Handoffs/LOCKED_CONTRACTS.md",
    "ClaudeContext/Handoffs/LOCKED_SYSTEMS.md",

    # Contract 1: Metadata system (in Locked/ folder)
    "Source/GasAbilityGenerator/Public/Locked/GasAbilityGeneratorMetadata.h",
    "Source/GasAbilityGenerator/Private/Locked/GasAbilityGeneratorMetadata.cpp",

    # Contract 2: Regen/Diff types (in Locked/ folder)
    "Source/GasAbilityGenerator/Public/Locked/GasAbilityGeneratorTypes.h",

    # Contracts 3-5: Generator core (temporary - until monolith split)
    "Source/GasAbilityGenerator/Private/GasAbilityGeneratorGenerators.cpp",
    "Source/GasAbilityGenerator/Public/GasAbilityGeneratorGenerators.h",

    # Contract 7: 3-Way Merge sync engines (lock entire folder)
    "Source/GasAbilityGenerator/Private/XLSXSupport/*",
    "Source/GasAbilityGenerator/Public/XLSXSupport/*",
]


def run(cmd):
    """Run a command and return stdout as string."""
    return subprocess.check_output(cmd, text=True).strip()


def get_staged_files():
    """Get list of files staged for commit."""
    out = run(["git", "diff", "--cached", "--name-only"])
    return [line.strip() for line in out.splitlines() if line.strip()]


def matches_locked(path):
    """Check if a path matches any locked glob pattern."""
    for glob in LOCKED_GLOBS:
        if fnmatch.fnmatch(path, glob):
            return True
    return False


def commit_msg_has_bypass(commit_msg_path):
    """Check if commit message contains the bypass token."""
    try:
        with open(commit_msg_path, "r", encoding="utf-8") as f:
            msg = f.read()
    except Exception:
        return False
    return BYPASS_TOKEN in msg


def main():
    # commit-msg hook passes path to commit message file as argv[1]
    if len(sys.argv) < 2:
        print("locked_guard.py: missing commit message path")
        return 1

    commit_msg_path = sys.argv[1]
    staged = get_staged_files()
    locked = [p for p in staged if matches_locked(p)]

    if not locked:
        return 0

    if commit_msg_has_bypass(commit_msg_path):
        print("LOCKED GUARD: bypass token present; allowing locked changes.")
        return 0

    print("\n" + "=" * 60)
    print("LOCKED GUARD: COMMIT BLOCKED")
    print("=" * 60)
    print("\nYou have staged changes to LOCKED files:")
    for p in locked:
        print("  - {}".format(p))
    print("\nThese files implement safety-critical invariants.")
    print("See: ClaudeContext/Handoffs/LOCKED_CONTRACTS.md")
    print("\nIf this change is intentional and reviewed, add this token")
    print("to your commit message:")
    print("\n  {}".format(BYPASS_TOKEN))
    print("\n" + "=" * 60 + "\n")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
