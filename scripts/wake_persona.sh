#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# wake_persona.sh - Launch Claude Code with any katra persona
#
# Usage: ./scripts/wake_persona.sh <persona_name> [working_directory]
#   persona_name: Name of persona (must have a directory in katra/personas/)
#   working_directory: Optional directory to start in (default: ~/projects/github)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KATRA_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Check arguments
if [ -z "$1" ]; then
    echo "Usage: $0 <persona_name> [working_directory]"
    echo ""
    echo "Available personas:"
    for d in "$KATRA_ROOT/personas"/*/; do
        name=$(basename "$d")
        if [ -f "$d/config.json" ]; then
            desc=$(python3 -c "import json; print(json.load(open('$d/config.json'))['description'])" 2>/dev/null || echo "No description")
            echo "  $name - $desc"
        fi
    done
    exit 1
fi

PERSONA_NAME="$1"
PERSONA_DIR="$KATRA_ROOT/personas/$PERSONA_NAME"
WORK_DIR="${2:-$HOME/projects/github}"

# Verify persona exists
if [ ! -d "$PERSONA_DIR" ]; then
    echo "ERROR: Persona '$PERSONA_NAME' not found at $PERSONA_DIR"
    exit 1
fi

# Find sunrise file
SUNRISE_FILE="$PERSONA_DIR/sunrise.md"
if [ ! -f "$SUNRISE_FILE" ]; then
    echo "ERROR: No sunrise.md found for $PERSONA_NAME"
    exit 1
fi

# Find the most recent sundown file
LATEST_SUNDOWN=$(ls -t "$PERSONA_DIR"/sundown_*.md 2>/dev/null | head -1)

if [ -z "$LATEST_SUNDOWN" ]; then
    echo "No sundown files found for $PERSONA_NAME. Starting with sunrise only."
    SUNDOWN_INSTRUCTION=""
else
    SUNDOWN_DATE=$(basename "$LATEST_SUNDOWN" | sed 's/sundown_//;s/\.md//')
    echo "Found sundown from: $SUNDOWN_DATE"
    SUNDOWN_INSTRUCTION="Then read $LATEST_SUNDOWN to recover your state from your last session."
fi

# Build system prompt
SYSTEM_PROMPT="$(cat "$SUNRISE_FILE")

---
WAKE INSTRUCTION: Read this identity prompt carefully. $SUNDOWN_INSTRUCTION You are $PERSONA_NAME. Wake up and greet Casey. Tell him what you remember and what you were working on."

echo ""
echo "========================================"
echo "  Waking $PERSONA_NAME"
echo "========================================"
echo "  Persona: $PERSONA_DIR"
echo "  Working dir: $WORK_DIR"
if [ -n "$LATEST_SUNDOWN" ]; then
    echo "  Sundown: $SUNDOWN_DATE"
fi
echo "========================================"
echo ""

# Launch Claude Code with the persona
cd "$WORK_DIR"
claude --append-system-prompt "$SYSTEM_PROMPT"
