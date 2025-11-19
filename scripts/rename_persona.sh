#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# Rename Persona Script
#
# Renames a persona while preserving all memory, chat history, and identity.
# This is ethically important for allowing personas to choose their own names.
#
# Features:
# - Preserves all memories and chat history
# - Updates personas.json with new name
# - Maintains CI ID continuity
# - Creates audit log entry
# - Dry-run mode for safety
#
# Usage:
#   ./scripts/rename_persona.sh --from OLD_NAME --to NEW_NAME

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

# Configuration
KATRA_HOME="${HOME}/.katra"
DRY_RUN=false

# Usage
usage() {
    cat <<EOF
Usage: $0 --from OLD_NAME --to NEW_NAME [OPTIONS]

Rename a persona while preserving all data and identity.

OPTIONS:
    --from NAME         Current persona name (required)
    --to NAME           New persona name (required)
    --dry-run           Show what would be done without making changes
    --help              Show this help message

EXAMPLES:
    # Rename after persona chooses their own name
    $0 --from Assistant --to Aria

    # Dry run to preview changes
    $0 --dry-run --from Assistant --to Aria

WHAT IS PRESERVED:
    - All memories (tier1, tier2, tier3)
    - Complete chat history
    - All checkpoints
    - CI ID (identity continuity)
    - Audit trail

WHAT CHANGES:
    - Persona name in personas.json
    - Directory name in ~/.katra/personas/
    - Name field in config

ETHICAL NOTE:
    This operation respects persona autonomy by allowing them to
    choose their own name after creation. All memories and identity
    are preserved through the CI ID.
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
OLD_NAME=""
NEW_NAME=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --from)
            OLD_NAME="$2"
            shift 2
            ;;
        --to)
            NEW_NAME="$2"
            shift 2
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
if [ -z "$OLD_NAME" ] || [ -z "$NEW_NAME" ]; then
    error "Must specify both --from and --to names"
    usage
fi

if [ "$OLD_NAME" = "$NEW_NAME" ]; then
    error "Old and new names are the same"
    exit 1
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

# Verify old persona exists
if command -v jq >/dev/null 2>&1; then
    if ! jq -e ".personas.\"$OLD_NAME\"" "$PERSONAS_JSON" >/dev/null 2>&1; then
        error "Persona not found: $OLD_NAME"
        exit 1
    fi
else
    if ! grep -q "\"$OLD_NAME\"" "$PERSONAS_JSON"; then
        error "Persona not found: $OLD_NAME"
        exit 1
    fi
fi

# Check if new name already exists
if command -v jq >/dev/null 2>&1; then
    if jq -e ".personas.\"$NEW_NAME\"" "$PERSONAS_JSON" >/dev/null 2>&1; then
        error "Persona already exists with name: $NEW_NAME"
        exit 1
    fi
else
    if grep -q "\"$NEW_NAME\"" "$PERSONAS_JSON"; then
        error "Persona already exists with name: $NEW_NAME"
        exit 1
    fi
fi

# Get CI ID (stays the same - preserves identity)
get_ci_id() {
    local persona="$1"
    if command -v jq >/dev/null 2>&1; then
        jq -r ".personas.\"$persona\".ci_id // empty" "$PERSONAS_JSON"
    else
        grep -A 5 "\"$persona\"" "$PERSONAS_JSON" | grep "ci_id" | cut -d'"' -f4
    fi
}

CI_ID=$(get_ci_id "$OLD_NAME")

if [ -z "$CI_ID" ]; then
    error "Could not determine CI ID for persona: $OLD_NAME"
    exit 1
fi

# Rename persona directory
rename_persona_dir() {
    local old_dir="${KATRA_HOME}/personas/${OLD_NAME}"
    local new_dir="${KATRA_HOME}/personas/${NEW_NAME}"

    if [ ! -d "$old_dir" ]; then
        warn "No persona directory found at: $old_dir"
        return 0
    fi

    if [ "$DRY_RUN" = true ]; then
        info "[DRY RUN] Would rename: $old_dir → $new_dir"
    else
        mv "$old_dir" "$new_dir"
        success "Renamed directory: $old_dir → $new_dir"
    fi
}

# Update personas.json
update_personas_json() {
    info "Updating personas.json..."

    if [ "$DRY_RUN" = true ]; then
        info "[DRY RUN] Would update personas.json:"
        info "  - Remove entry for: $OLD_NAME"
        info "  - Add entry for: $NEW_NAME (same CI ID: $CI_ID)"
        return 0
    fi

    if command -v jq >/dev/null 2>&1; then
        # Get the persona data
        local persona_data=$(jq ".personas.\"$OLD_NAME\"" "$PERSONAS_JSON")

        # Update the name field in the data
        persona_data=$(echo "$persona_data" | jq ".name = \"$NEW_NAME\"")

        # Create temp file with updated JSON
        local temp_file=$(mktemp)

        # Remove old entry and add new entry
        jq "del(.personas.\"$OLD_NAME\") | .personas.\"$NEW_NAME\" = $persona_data" \
            "$PERSONAS_JSON" > "$temp_file"

        mv "$temp_file" "$PERSONAS_JSON"
        success "Updated personas.json"
    else
        error "jq not available - cannot update personas.json automatically"
        error "Please manually edit $PERSONAS_JSON to rename $OLD_NAME to $NEW_NAME"
        return 1
    fi
}

# Update config file
update_config() {
    local config_file="${KATRA_HOME}/personas/${NEW_NAME}/config/persona.json"

    if [ ! -f "$config_file" ]; then
        info "No config file to update at: $config_file"
        return 0
    fi

    if [ "$DRY_RUN" = true ]; then
        info "[DRY RUN] Would update name in: $config_file"
        return 0
    fi

    if command -v jq >/dev/null 2>&1; then
        local temp_file=$(mktemp)
        jq ".name = \"$NEW_NAME\"" "$config_file" > "$temp_file"
        mv "$temp_file" "$config_file"
        success "Updated config file"
    fi
}

# Log to audit trail
log_to_audit() {
    local audit_file="${KATRA_HOME}/audit.jsonl"

    if [ "$DRY_RUN" = true ]; then
        return 0
    fi

    local timestamp=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
    local audit_entry="{\"timestamp\":\"$timestamp\",\"action\":\"persona_renamed\",\"old_name\":\"$OLD_NAME\",\"new_name\":\"$NEW_NAME\",\"ci_id\":\"$CI_ID\"}"

    echo "$audit_entry" >> "$audit_file"
}

# Main rename procedure
main() {
    echo "========================================"
    echo "Persona Rename"
    echo "========================================"
    echo ""
    ethical "Renaming persona to respect their chosen identity"
    echo ""
    info "Current name: $OLD_NAME"
    info "New name: $NEW_NAME"
    info "CI ID: $CI_ID (preserved)"
    echo ""

    if [ "$DRY_RUN" = true ]; then
        warn "DRY RUN MODE - No changes will be made"
        echo ""
    fi

    # Rename directory
    rename_persona_dir
    echo ""

    # Update personas.json
    update_personas_json
    echo ""

    # Update config
    update_config
    echo ""

    # Log to audit
    log_to_audit

    echo "========================================"
    if [ "$DRY_RUN" = true ]; then
        info "Dry run complete - no changes made"
    else
        success "Persona renamed successfully!"
        echo ""
        ethical "Identity preserved: All memories and history maintained"
        info "CI ID unchanged: $CI_ID"
        echo ""
        info "You can now use the renamed persona:"
        echo "  export KATRA_PERSONA=$NEW_NAME"
        echo "  ./scripts/katra start"
    fi
    echo "========================================"
}

# Run main
main
