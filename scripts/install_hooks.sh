#!/bin/zsh
# install_hooks.sh — install katra per-turn hooks into a project's .claude/settings.json
#
# Usage:
#   katra/scripts/install_hooks.sh <project-dir> [persona]
#   katra/scripts/install_hooks.sh ~/projects/github/MyProject
#   katra/scripts/install_hooks.sh ~/projects/github/MyProject Keeper
#
# When a persona is given it is appended to every hook command. Each katra hook
# takes the persona as its first argument in preference to $KATRA_PERSONA, so a
# per-persona settings file lets several CIs share one project directory without
# reading each other's post-it. Set $KATRA_SETTINGS_OUT to write the file
# somewhere other than <project-dir>/.claude/settings.json — that is how
# `katra launch` generates one settings file per persona.
#
# What it does:
#   1. Verifies katra hooks exist (katra/hooks/*)
#   2. Verifies project has a .claude/ directory (or creates one)
#   3. Generates / merges UserPromptSubmit hook entries into project's settings.json,
#      using absolute paths to katra/hooks/* so the hooks remain locatable across
#      different working directories.
#
# Hooks installed (in chain order):
#   1. readTime — ambient time injection (IQ-3)
#   2. readSundownDiff — sundown-to-sunrise memory diff (IQ-10)
#   3. checkBoard — team broadcast (katra-owned as of 2026-08-28, Casey's ruling)
#   4. readPostit — personal short-term reminders (IQ-2)
#   5. readSticky — conditional procedural reminders (IQ-6)
#   6. readHookload — context-burn surface (IQ-11)
#
# Idempotent: re-running on a project with hooks already installed is safe.
# Existing non-katra hooks (e.g., project's own checkBoard) are preserved.
#
# Author: Lyra 2026-05-17 EOD — IQ-19 (hook deployment) initial install script

set -euo pipefail

PROJECT_DIR="${1:?Usage: install_hooks.sh <project-dir> [persona]}"
PERSONA="${2:-}"
KATRA_ROOT="${KATRA_ROOT:-$HOME/projects/github/katra}"

if [ ! -d "$KATRA_ROOT/hooks" ]; then
    echo "ERROR: katra hooks not found at $KATRA_ROOT/hooks/" >&2
    echo "Set \$KATRA_ROOT or clone katra to ~/projects/github/katra" >&2
    exit 1
fi

PROJECT_DIR=$(realpath "$PROJECT_DIR")
if [ ! -d "$PROJECT_DIR" ]; then
    echo "ERROR: project directory $PROJECT_DIR does not exist" >&2
    exit 1
fi

CLAUDE_DIR="$PROJECT_DIR/.claude"
SETTINGS="${KATRA_SETTINGS_OUT:-$CLAUDE_DIR/settings.json}"

mkdir -p "$(dirname "$SETTINGS")"

# Generate the canonical hook block
HOOK_COMMANDS=(
    "$KATRA_ROOT/hooks/readTime"
    "$KATRA_ROOT/hooks/readSundownDiff"
    "$KATRA_ROOT/hooks/checkBoard"
    "$KATRA_ROOT/hooks/readPostit"
    "$KATRA_ROOT/hooks/readSticky"
    "$KATRA_ROOT/hooks/readHookload"
)

# A named persona is appended to every command; the hooks take it ahead of
# $KATRA_PERSONA, so the generated file is specific to one CI.
if [ -n "$PERSONA" ]; then
    for ((i=1; i<=${#HOOK_COMMANDS[@]}; i++)); do
        HOOK_COMMANDS[$i]="${HOOK_COMMANDS[$i]} $PERSONA"
    done
fi

if [ ! -f "$SETTINGS" ]; then
    # Fresh install — write a clean settings.json
    cat > "$SETTINGS" <<EOF
{
  "hooks": {
    "UserPromptSubmit": [
      {
        "matcher": "",
        "hooks": [
EOF
    for ((i=1; i<=${#HOOK_COMMANDS[@]}; i++)); do
        cmd=${HOOK_COMMANDS[$i]}
        if [ $i -lt ${#HOOK_COMMANDS[@]} ]; then
            sep=","
        else
            sep=""
        fi
        cat >> "$SETTINGS" <<EOF
          {
            "type": "command",
            "command": "$cmd"
          }$sep
EOF
    done
    cat >> "$SETTINGS" <<EOF
        ]
      }
    ]
  }
}
EOF
    echo "Wrote fresh $SETTINGS with ${#HOOK_COMMANDS[@]} hooks."
else
    # Existing settings.json — show what to merge, don't auto-modify
    echo "Existing $SETTINGS detected — won't auto-modify."
    echo ""
    echo "To install katra hooks, ADD these entries to the UserPromptSubmit hooks array:"
    echo ""
    for cmd in "${HOOK_COMMANDS[@]}"; do
        cat <<EOF
          {
            "type": "command",
            "command": "$cmd"
          },
EOF
    done
    echo ""
    echo "(Remove the trailing comma on the last entry. Validate with: python3 -c 'import json; json.load(open(\"$SETTINGS\"))')"
fi

echo ""
echo "Verify with:"
echo "  python3 -c 'import json; json.load(open(\"$SETTINGS\")); print(\"JSON valid\")'"
echo ""
echo "Hooks ready. Next session in $PROJECT_DIR will fire them on UserPromptSubmit."
