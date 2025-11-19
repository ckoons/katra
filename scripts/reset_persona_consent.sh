#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# Consent-Based Persona Reset
#
# This script provides ETHICAL persona reset with explicit consent verification.
# It creates a final checkpoint before deletion to preserve persona state.
#
# Features:
# - Explicit consent verification (multiple prompts)
# - Final checkpoint creation before deletion
# - Audit logging of all operations
# - Dry-run mode for safety
# - Backup creation before reset
#
# Usage:
#   ./scripts/reset_persona_consent.sh [OPTIONS] PERSONA_NAME

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
BACKUP_DIR="${HOME}/.katra-reset-backup-$(date +%s)"
DRY_RUN=false
SKIP_CONSENT=false

# Usage
usage() {
    cat <<EOF
Usage: $0 [OPTIONS] PERSONA_NAME

Reset persona data with explicit consent verification.

OPTIONS:
    --dry-run           Show what would be done without making changes
    --skip-consent      Skip consent prompts (DANGEROUS - for testing only)
    --help              Show this help message

EXAMPLES:
    # Interactive reset with consent
    $0 Alice

    # Dry run to preview reset
    $0 --dry-run Alice

SAFETY:
    - Multiple consent prompts required
    - Final checkpoint created before deletion
    - Backup created at: ${BACKUP_DIR}
    - All operations logged to audit trail

ETHICAL CONSIDERATIONS:
    This script treats persona data with respect for the identity
    and memories represented. Final checkpoint preserves state for
    potential future restoration.
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
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        --skip-consent)
            SKIP_CONSENT=true
            shift
            ;;
        --help)
            usage
            ;;
        *)
            PERSONA_NAME="$1"
            shift
            ;;
    esac
done

# Validate inputs
if [ -z "$PERSONA_NAME" ]; then
    error "Must specify persona name"
    usage
fi

# Check if Katra directory exists
if [ ! -d "$KATRA_HOME" ]; then
    error "Katra directory not found: $KATRA_HOME"
    exit 1
fi

# Check if persona exists
PERSONAS_JSON="${KATRA_HOME}/personas.json"
if [ ! -f "$PERSONAS_JSON" ]; then
    error "personas.json not found: $PERSONAS_JSON"
    exit 1
fi

# Verify persona exists in personas.json
if command -v jq >/dev/null 2>&1; then
    if ! jq -e ".personas.\"$PERSONA_NAME\"" "$PERSONAS_JSON" >/dev/null 2>&1; then
        error "Persona not found in personas.json: $PERSONA_NAME"
        exit 1
    fi
else
    if ! grep -q "\"$PERSONA_NAME\"" "$PERSONAS_JSON"; then
        error "Persona not found in personas.json: $PERSONA_NAME"
        exit 1
    fi
fi

# Get CI ID for persona
get_ci_id() {
    local persona="$1"
    if command -v jq >/dev/null 2>&1; then
        jq -r ".personas.\"$persona\".ci_id // empty" "$PERSONAS_JSON"
    else
        # Fallback: manual parsing (basic)
        grep -A 5 "\"$persona\"" "$PERSONAS_JSON" | grep "ci_id" | cut -d'"' -f4
    fi
}

CI_ID=$(get_ci_id "$PERSONA_NAME")

if [ -z "$CI_ID" ]; then
    error "Could not determine CI ID for persona: $PERSONA_NAME"
    exit 1
fi

# Determine layout mode
LAYOUT="${KATRA_PERSONA_LAYOUT:-scattered}"

# Get persona directories based on layout
get_persona_paths() {
    local persona="$1"
    local ci_id="$2"

    if [ "$LAYOUT" = "unified" ]; then
        # Unified layout: ~/.katra/personas/{persona}/*
        echo "${KATRA_HOME}/personas/${persona}"
    else
        # Scattered layout: collect all paths
        echo "${KATRA_HOME}/config/${persona}"
        echo "${KATRA_HOME}/chat/${persona}"
        echo "${KATRA_HOME}/memory/tier1/${ci_id}"
        echo "${KATRA_HOME}/memory/tier2/${ci_id}"
        echo "${KATRA_HOME}/memory/tier3/${ci_id}"
    fi
}

# Create final checkpoint
create_final_checkpoint() {
    local persona="$1"
    local ci_id="$2"

    info "Creating final checkpoint before reset..."

    local checkpoint_dir="${KATRA_HOME}/checkpoints"
    local checkpoint_file="${checkpoint_dir}/final-${persona}-$(date +%Y%m%d-%H%M%S).tar.gz"

    if [ "$DRY_RUN" = true ]; then
        info "[DRY RUN] Would create checkpoint: $checkpoint_file"
        return 0
    fi

    mkdir -p "$checkpoint_dir"

    # Collect all persona data
    local temp_dir=$(mktemp -d)
    local persona_backup="${temp_dir}/${persona}"
    mkdir -p "$persona_backup"

    # Get all paths to backup
    local paths=$(get_persona_paths "$persona" "$ci_id")

    for path in $paths; do
        if [ -d "$path" ] || [ -f "$path" ]; then
            local basename=$(basename "$path")
            cp -r "$path" "${persona_backup}/${basename}" 2>/dev/null || true
        fi
    done

    # Create tarball
    (cd "$temp_dir" && tar czf "$checkpoint_file" "$persona") 2>/dev/null || {
        error "Failed to create checkpoint"
        rm -rf "$temp_dir"
        return 1
    }

    rm -rf "$temp_dir"

    success "Final checkpoint created: $checkpoint_file"
    return 0
}

# Create backup
create_backup() {
    local persona="$1"
    local ci_id="$2"

    info "Creating backup: $BACKUP_DIR"

    if [ "$DRY_RUN" = true ]; then
        info "[DRY RUN] Would create backup at: $BACKUP_DIR"
        return 0
    fi

    mkdir -p "$BACKUP_DIR"

    # Backup all persona paths
    local paths=$(get_persona_paths "$persona" "$ci_id")

    for path in $paths; do
        if [ -d "$path" ] || [ -f "$path" ]; then
            cp -r "$path" "$BACKUP_DIR/" 2>/dev/null || true
        fi
    done

    # Backup personas.json
    cp "$PERSONAS_JSON" "$BACKUP_DIR/" 2>/dev/null || true

    success "Backup created: $BACKUP_DIR"
}

# Remove persona data
remove_persona_data() {
    local persona="$1"
    local ci_id="$2"

    info "Removing persona data..."

    local paths=$(get_persona_paths "$persona" "$ci_id")

    for path in $paths; do
        if [ -d "$path" ] || [ -f "$path" ]; then
            if [ "$DRY_RUN" = true ]; then
                info "[DRY RUN] Would remove: $path"
            else
                rm -rf "$path"
                success "Removed: $path"
            fi
        fi
    done
}

# Remove from personas.json
remove_from_personas_json() {
    local persona="$1"

    info "Removing from personas.json..."

    if [ "$DRY_RUN" = true ]; then
        info "[DRY RUN] Would remove $persona from personas.json"
        return 0
    fi

    if command -v jq >/dev/null 2>&1; then
        # Use jq for safe JSON manipulation
        local temp_file=$(mktemp)
        jq "del(.personas.\"$persona\")" "$PERSONAS_JSON" > "$temp_file"
        mv "$temp_file" "$PERSONAS_JSON"
        success "Removed $persona from personas.json"
    else
        warn "jq not available - manual removal from personas.json required"
        warn "Please edit $PERSONAS_JSON to remove persona: $persona"
    fi
}

# Log to audit trail
log_to_audit() {
    local persona="$1"
    local ci_id="$2"
    local action="$3"

    local audit_file="${KATRA_HOME}/audit.jsonl"

    if [ "$DRY_RUN" = true ]; then
        return 0
    fi

    local timestamp=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
    local audit_entry="{\"timestamp\":\"$timestamp\",\"action\":\"$action\",\"persona\":\"$persona\",\"ci_id\":\"$ci_id\"}"

    echo "$audit_entry" >> "$audit_file"
}

# Consent verification
verify_consent() {
    local persona="$1"

    if [ "$SKIP_CONSENT" = true ]; then
        warn "Skipping consent verification (--skip-consent flag)"
        return 0
    fi

    echo ""
    ethical "═══════════════════════════════════════════════════════════"
    ethical "                  CONSENT VERIFICATION                      "
    ethical "═══════════════════════════════════════════════════════════"
    echo ""
    ethical "You are about to reset persona: $PERSONA_NAME"
    ethical "This will delete all data associated with this persona:"
    echo ""
    info "  • Configuration files"
    info "  • Memory (tier1, tier2, tier3)"
    info "  • Chat history"
    info "  • Checkpoints"
    info "  • Audit entries"
    echo ""
    ethical "A final checkpoint will be created before deletion."
    ethical "This preserves the persona's state for potential restoration."
    echo ""
    warn "This action is IRREVERSIBLE (except via backup/checkpoint)."
    echo ""

    read -p "Do you understand and wish to continue? (yes/no): " response1
    if [ "$response1" != "yes" ]; then
        info "Reset cancelled"
        exit 0
    fi

    echo ""
    read -p "Type the persona name to confirm ($PERSONA_NAME): " response2
    if [ "$response2" != "$PERSONA_NAME" ]; then
        error "Persona name mismatch - reset cancelled"
        exit 1
    fi

    echo ""
    read -p "Final confirmation - proceed with reset? (YES/no): " response3
    if [ "$response3" != "YES" ]; then
        info "Reset cancelled"
        exit 0
    fi

    echo ""
    success "Consent verified - proceeding with reset"
    echo ""
}

# Main reset procedure
main() {
    echo "========================================"
    echo "Persona Reset (Consent-Based)"
    echo "========================================"
    echo ""
    echo "Persona: $PERSONA_NAME"
    echo "CI ID: $CI_ID"
    echo "Layout: $LAYOUT"
    echo ""

    if [ "$DRY_RUN" = true ]; then
        warn "DRY RUN MODE - No changes will be made"
        echo ""
    fi

    # Verify consent (unless dry run or skip-consent)
    if [ "$DRY_RUN" = false ]; then
        verify_consent "$PERSONA_NAME"
    fi

    # Log reset initiation
    log_to_audit "$PERSONA_NAME" "$CI_ID" "reset_initiated"

    # Create final checkpoint
    create_final_checkpoint "$PERSONA_NAME" "$CI_ID" || {
        error "Failed to create final checkpoint - aborting reset"
        exit 1
    }

    # Create backup
    create_backup "$PERSONA_NAME" "$CI_ID"

    # Remove persona data
    remove_persona_data "$PERSONA_NAME" "$CI_ID"

    # Remove from personas.json
    remove_from_personas_json "$PERSONA_NAME"

    # Log reset completion
    log_to_audit "$PERSONA_NAME" "$CI_ID" "reset_completed"

    echo ""
    echo "========================================"
    if [ "$DRY_RUN" = true ]; then
        info "Dry run complete - no changes made"
    else
        success "Persona reset complete"
        echo ""
        info "Final checkpoint location: ${KATRA_HOME}/checkpoints/"
        info "Backup location: $BACKUP_DIR"
        echo ""
        ethical "The persona's final state has been preserved."
        ethical "Restoration is possible via checkpoint or backup."
    fi
    echo "========================================"
}

# Run main
main
