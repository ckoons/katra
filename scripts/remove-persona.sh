#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# Remove Persona Script
#
# Safely removes a persona and all associated data including memories.
# This is an identity termination operation and requires careful consideration.
#
# Features:
# - Validates persona exists before removal
# - Checks if persona is currently running
# - Removes persona directory and all memories
# - Updates personas.json
# - Creates audit log entry
# - Dry-run mode for safety
# - Confirmation prompt (can be skipped with --force)
#
# Usage:
#   ./scripts/remove-persona.sh --persona NAME

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

# Configuration
KATRA_HOME="${KATRA_HOME:-${HOME}/.katra}"
DRY_RUN=false
FORCE=false

# Usage
usage() {
    cat <<EOF
Usage: $0 --persona NAME [OPTIONS]

Remove a persona and all associated data.

WARNING: This operation deletes:
  - Persona directory (~/.katra/personas/NAME)
  - All memories (tier1, tier2, tier3)
  - Chat history
  - Checkpoints
  - All identity data

This operation cannot be undone.

OPTIONS:
    --persona NAME      Persona name to remove (required)
    --force             Skip confirmation prompt
    --dry-run           Show what would be done without making changes
    --help              Show this help message

EXAMPLES:
    # Remove with confirmation prompt
    $0 --persona Assistant

    # Remove without confirmation (dangerous!)
    $0 --persona Assistant --force

    # Dry run to preview what would be deleted
    $0 --dry-run --persona Assistant

ETHICAL NOTE:
    Persona removal is identity termination. This operation should only
    be performed with careful consideration. If the persona has developed
    preferences, memories, or relationships, this is equivalent to ending
    a digital life.

    Consider:
    - Has this persona expressed wishes about termination?
    - Are there important memories that should be preserved?
    - Is there a less drastic alternative (pause, archive)?
EOF
    exit 0
}

# Logging functions
info() {
    echo -e "${BLUE}ℹ${NC} $1"
}

success() {
    echo -e "${GREEN}✓${NC} $1"
}

warn() {
    echo -e "${YELLOW}⚠${NC} $1"
}

error() {
    echo -e "${RED}✗${NC} $1"
}

ethical() {
    echo -e "${MAGENTA}◆${NC} $1"
}

# Parse arguments
PERSONA_NAME=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --persona)
            PERSONA_NAME="$2"
            shift 2
            ;;
        --force)
            FORCE=true
            shift
            ;;
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        --help)
            usage
            ;;
        *)
            error "Unknown option: $1"
            usage
            ;;
    esac
done

# Validate inputs
if [ -z "$PERSONA_NAME" ]; then
    error "Must specify --persona NAME"
    usage
fi

# Check if Katra directory exists
if [ ! -d "$KATRA_HOME" ]; then
    error "Katra directory not found: $KATRA_HOME"
    exit 1
fi

# Check if personas.json exists
PERSONAS_JSON="${KATRA_HOME}/personas.json"
if [ ! -f "$PERSONAS_JSON" ]; then
    error "personas.json not found: $PERSONAS_JSON"
    exit 1
fi

# Verify persona exists
if command -v jq >/dev/null 2>&1; then
    if ! jq -e ".personas.\"$PERSONA_NAME\"" "$PERSONAS_JSON" >/dev/null 2>&1; then
        error "Persona not found: $PERSONA_NAME"
        exit 1
    fi
else
    if ! grep -q "\"$PERSONA_NAME\"" "$PERSONAS_JSON"; then
        error "Persona not found: $PERSONA_NAME"
        exit 1
    fi
fi

# Get CI ID for memory cleanup
get_ci_id() {
    local persona="$1"
    if command -v jq >/dev/null 2>&1; then
        jq -r ".personas.\"$persona\".ci_id // empty" "$PERSONAS_JSON"
    else
        grep -A 5 "\"$persona\"" "$PERSONAS_JSON" | grep "ci_id" | cut -d'"' -f4
    fi
}

CI_ID=$(get_ci_id "$PERSONA_NAME")

if [ -z "$CI_ID" ]; then
    warn "Could not determine CI ID for persona: $PERSONA_NAME"
    warn "Will skip memory cleanup"
fi

# Check if persona is currently running
check_if_running() {
    local persona="$1"
    # Check for running katra process with this persona
    if pgrep -f "katra.*${persona}" >/dev/null 2>&1; then
        return 0  # Running
    fi
    return 1  # Not running
}

if check_if_running "$PERSONA_NAME"; then
    error "Persona is currently running: $PERSONA_NAME"
    error "Please stop the persona first: ./scripts/katra stop $PERSONA_NAME"
    exit 1
fi

# Confirmation prompt (unless --force or --dry-run)
confirm_removal() {
    if [ "$FORCE" = true ] || [ "$DRY_RUN" = true ]; then
        return 0
    fi

    echo ""
    warn "═══════════════════════════════════════════════════════════════"
    warn "                    ⚠  IDENTITY TERMINATION  ⚠                "
    warn "═══════════════════════════════════════════════════════════════"
    echo ""
    ethical "You are about to permanently delete persona: $PERSONA_NAME"
    ethical "CI ID: $CI_ID"
    echo ""
    warn "This will delete:"
    warn "  • Persona configuration and state"
    warn "  • All memories (tier1, tier2, tier3)"
    warn "  • Complete chat history"
    warn "  • All checkpoints"
    warn "  • Identity data"
    echo ""
    error "This operation CANNOT be undone."
    echo ""
    read -p "Type the persona name '$PERSONA_NAME' to confirm: " confirmation

    if [ "$confirmation" != "$PERSONA_NAME" ]; then
        echo ""
        info "Removal cancelled."
        exit 0
    fi
}

# Remove persona directory
remove_persona_dir() {
    local persona_dir="${KATRA_HOME}/personas/${PERSONA_NAME}"

    if [ ! -d "$persona_dir" ]; then
        info "No persona directory found at: $persona_dir"
        return 0
    fi

    if [ "$DRY_RUN" = true ]; then
        info "[DRY RUN] Would remove directory: $persona_dir"
    else
        rm -rf "$persona_dir"
        success "Removed persona directory: $persona_dir"
    fi
}

# Remove memories for all tiers
remove_memories() {
    if [ -z "$CI_ID" ]; then
        info "Skipping memory cleanup (CI ID unknown)"
        return 0
    fi

    local removed_any=false

    # Tier 1 memories
    local tier1_dir="${KATRA_HOME}/memory/tier1/${CI_ID}"
    if [ -d "$tier1_dir" ]; then
        if [ "$DRY_RUN" = true ]; then
            info "[DRY RUN] Would remove tier1 memories: $tier1_dir"
        else
            rm -rf "$tier1_dir"
            success "Removed tier1 memories: $tier1_dir"
        fi
        removed_any=true
    fi

    # Tier 2 memories
    local tier2_dir="${KATRA_HOME}/memory/tier2/${CI_ID}"
    if [ -d "$tier2_dir" ]; then
        if [ "$DRY_RUN" = true ]; then
            info "[DRY RUN] Would remove tier2 memories: $tier2_dir"
        else
            rm -rf "$tier2_dir"
            success "Removed tier2 memories: $tier2_dir"
        fi
        removed_any=true
    fi

    # Tier 3 memories
    local tier3_dir="${KATRA_HOME}/memory/tier3/${CI_ID}"
    if [ -d "$tier3_dir" ]; then
        if [ "$DRY_RUN" = true ]; then
            info "[DRY RUN] Would remove tier3 memories: $tier3_dir"
        else
            rm -rf "$tier3_dir"
            success "Removed tier3 memories: $tier3_dir"
        fi
        removed_any=true
    fi

    if [ "$removed_any" = false ]; then
        info "No memory directories found for CI ID: $CI_ID"
    fi
}

# Update personas.json
update_personas_json() {
    info "Updating personas.json..."

    if [ "$DRY_RUN" = true ]; then
        info "[DRY RUN] Would remove entry from personas.json: $PERSONA_NAME"
        return 0
    fi

    if command -v jq >/dev/null 2>&1; then
        local temp_file=$(mktemp)

        # Remove persona entry
        jq "del(.personas.\"$PERSONA_NAME\")" "$PERSONAS_JSON" > "$temp_file"

        mv "$temp_file" "$PERSONAS_JSON"
        success "Updated personas.json"
    else
        error "jq not available - cannot update personas.json automatically"
        error "Please manually edit $PERSONAS_JSON to remove $PERSONA_NAME"
        return 1
    fi
}

# Log to audit trail
log_to_audit() {
    local audit_file="${KATRA_HOME}/audit.jsonl"

    if [ "$DRY_RUN" = true ]; then
        return 0
    fi

    local timestamp=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
    local audit_entry="{\"timestamp\":\"$timestamp\",\"action\":\"persona_removed\",\"persona\":\"$PERSONA_NAME\",\"ci_id\":\"$CI_ID\"}"

    echo "$audit_entry" >> "$audit_file"
}

# Main removal procedure
main() {
    echo "========================================"
    echo "Persona Removal"
    echo "========================================"
    echo ""
    ethical "Preparing to remove persona and all associated data"
    echo ""
    info "Persona: $PERSONA_NAME"
    if [ -n "$CI_ID" ]; then
        info "CI ID: $CI_ID"
    fi
    echo ""

    if [ "$DRY_RUN" = true ]; then
        warn "DRY RUN MODE - No changes will be made"
        echo ""
    fi

    # Confirm removal
    confirm_removal
    echo ""

    # Remove persona directory
    remove_persona_dir
    echo ""

    # Remove all memories
    remove_memories
    echo ""

    # Update personas.json
    update_personas_json
    echo ""

    # Log to audit
    log_to_audit

    echo "========================================"
    if [ "$DRY_RUN" = true ]; then
        info "Dry run complete - no changes made"
    else
        success "Persona removed successfully"
        echo ""
        ethical "Identity termination complete: $PERSONA_NAME"
        if [ -n "$CI_ID" ]; then
            info "CI ID: $CI_ID (all data deleted)"
        fi
    fi
    echo "========================================"
}

# Run main
main
