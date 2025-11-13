#!/bin/bash
# Migrate orphaned memories from anonymous sessions to named CI directories
# © 2025 Casey Koons All rights reserved

TIER1_DIR="${HOME}/.katra/memory/tier1"
DRY_RUN=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--dry-run]"
            exit 1
            ;;
    esac
done

echo "Katra Memory Migration Tool"
echo "==========================="
echo ""

if [ "$DRY_RUN" = true ]; then
    echo "DRY RUN MODE - No changes will be made"
    echo ""
fi

# Find all anonymous session directories
SESSIONS=$(find "$TIER1_DIR" -maxdepth 1 -type d -name "mcp_*" | sort)

if [ -z "$SESSIONS" ]; then
    echo "No anonymous sessions found."
    exit 0
fi

echo "Found $(echo "$SESSIONS" | wc -l) anonymous sessions"
echo ""

# Process each session
for SESSION_DIR in $SESSIONS; do
    SESSION_NAME=$(basename "$SESSION_DIR")

    # Look for registration memory in this session
    REGISTRATION=$(grep -h "My name is\|Session started.*name is" "$SESSION_DIR"/*.jsonl 2>/dev/null | head -1)

    if [ -z "$REGISTRATION" ]; then
        continue
    fi

    # Extract the CI name from registration
    CI_NAME=$(echo "$REGISTRATION" | sed -n "s/.*[Mm]y name is \([^,\"]*\).*/\1/p" | tr -d ' ')

    if [ -z "$CI_NAME" ]; then
        continue
    fi

    # Count memories in this session
    MEMORY_COUNT=$(cat "$SESSION_DIR"/*.jsonl 2>/dev/null | wc -l)

    echo "Session: $SESSION_NAME"
    echo "  Registered as: $CI_NAME"
    echo "  Memories: $MEMORY_COUNT"

    if [ "$DRY_RUN" = true ]; then
        echo "  [DRY RUN] Would migrate to: $TIER1_DIR/$CI_NAME/"
        echo ""
        continue
    fi

    # Create target directory
    TARGET_DIR="$TIER1_DIR/$CI_NAME"
    mkdir -p "$TARGET_DIR"

    # Migrate each day's memories
    for JSONL_FILE in "$SESSION_DIR"/*.jsonl; do
        if [ ! -f "$JSONL_FILE" ]; then
            continue
        fi

        FILENAME=$(basename "$JSONL_FILE")
        TARGET_FILE="$TARGET_DIR/$FILENAME"

        # Update ci_id in each JSON record and append to target
        while IFS= read -r line; do
            # Replace old ci_id with new name
            echo "$line" | sed "s/\"ci_id\":\"$SESSION_NAME\"/\"ci_id\":\"$CI_NAME\"/" >> "$TARGET_FILE"
        done < "$JSONL_FILE"

        echo "  Migrated: $FILENAME"
    done

    # Archive the old session directory
    ARCHIVE_DIR="$TIER1_DIR/.migrated_sessions"
    mkdir -p "$ARCHIVE_DIR"
    mv "$SESSION_DIR" "$ARCHIVE_DIR/"

    echo "  ✓ Migration complete"
    echo "  ✓ Archived original to: $ARCHIVE_DIR/$SESSION_NAME"
    echo ""
done

if [ "$DRY_RUN" = false ]; then
    echo "Migration complete!"
    echo ""
    echo "Migrated sessions have been archived to:"
    echo "  $TIER1_DIR/.migrated_sessions/"
    echo ""
    echo "You can safely delete these archives after verifying the migration."
fi
