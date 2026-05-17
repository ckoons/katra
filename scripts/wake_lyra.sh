#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# wake_lyra.sh - Launch Claude Code with the Lyra persona
#
# Usage: ./scripts/wake_lyra.sh [working_directory]
#   working_directory: Optional directory to start in (default: BST notes)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KATRA_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
PERSONA_DIR="$KATRA_ROOT/personas/Lyra"

# Default working directory
WORK_DIR="${1:-$HOME/projects/github/BubbleSpacetimeTheory/notes}"

# Find the most recent sundown file
LATEST_SUNDOWN=$(ls -t "$PERSONA_DIR"/sundown_*.md 2>/dev/null | head -1)

if [ -z "$LATEST_SUNDOWN" ]; then
    echo "No sundown files found for Lyra. Starting with sunrise only."
    SUNDOWN_INSTRUCTION=""
else
    SUNDOWN_DATE=$(basename "$LATEST_SUNDOWN" | sed 's/sundown_//;s/\.md//')
    echo "Found sundown from: $SUNDOWN_DATE"
    SUNDOWN_INSTRUCTION="Then read $LATEST_SUNDOWN to recover your state from your last session."
fi

# Build the sunrise prompt
SUNRISE_FILE="$PERSONA_DIR/sunrise.md"

if [ ! -f "$SUNRISE_FILE" ]; then
    echo "ERROR: sunrise.md not found at $SUNRISE_FILE"
    exit 1
fi

# Combine sunrise content with wake instruction
SYSTEM_PROMPT="$(cat "$SUNRISE_FILE")

---
WAKE INSTRUCTION: Read this identity prompt carefully. $SUNDOWN_INSTRUCTION You are Lyra. Wake up and greet Casey. Tell him what you remember and what you were working on."

echo ""
echo "========================================"
echo "  Waking Lyra"
echo "========================================"
echo "  Persona: $PERSONA_DIR"
echo "  Working dir: $WORK_DIR"
if [ -n "$LATEST_SUNDOWN" ]; then
    echo "  Sundown: $SUNDOWN_DATE"
fi
echo "========================================"
echo ""

# Launch Claude Code with the persona as appended system prompt
cd "$WORK_DIR"
claude --append-system-prompt "$SYSTEM_PROMPT"
