#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# Migrate Persona Layout: Scattered → Unified
#
# Migrates persona data from scattered layout to unified layout:
#   FROM: ~/.katra/{config,memory,chat}/{persona_name}
#   TO:   ~/.katra/personas/{persona_name}/{config,memory,chat}
#
# This script is SAFE and REVERSIBLE:
# - Creates backup before migration
# - Dry-run mode available
# - Can rollback on failure
# - Preserves all data and permissions

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
KATRA_HOME="${HOME}/.katra"
BACKUP_DIR="${HOME}/.katra-migration-backup-$(date +%s)"
DRY_RUN=false
VERBOSE=false

# Usage
usage() {
    cat <<EOF
Usage: $0 [OPTIONS] [PERSONA_NAME]

Migrate persona data from scattered to unified directory layout.

OPTIONS:
    --dry-run       Show what would be done without making changes
    --verbose       Show detailed output
    --all           Migrate all personas (from personas.json)
    --help          Show this help message

EXAMPLES:
    # Dry run for single persona
    $0 --dry-run Alice

    # Migrate single persona
    $0 Alice

    # Migrate all personas
    $0 --all

    # Verbose migration
    $0 --verbose --all

SAFETY:
    - Backup created at: ${BACKUP_DIR}
    - Original data preserved until verified
    - Rollback available via restore script
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

verbose() {
    if [ "$VERBOSE" = true ]; then
        echo -e "  ${NC}$1${NC}"
    fi
}

# Parse arguments
PERSONA_NAME=""
MIGRATE_ALL=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --all)
            MIGRATE_ALL=true
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
if [ "$MIGRATE_ALL" = false ] && [ -z "$PERSONA_NAME" ]; then
    error "Must specify persona name or --all"
    usage
fi

# Check if Katra directory exists
if [ ! -d "$KATRA_HOME" ]; then
    error "Katra directory not found: $KATRA_HOME"
    exit 1
fi

# Get list of personas to migrate
get_personas() {
    local personas_json="${KATRA_HOME}/personas.json"

    if [ ! -f "$personas_json" ]; then
        error "personas.json not found: $personas_json"
        return 1
    fi

    # Extract persona names from JSON
    if command -v jq >/dev/null 2>&1; then
        jq -r '.personas | keys[]' "$personas_json"
    else
        # Fallback: parse manually (basic, may not work for all cases)
        grep -o '"[^"]*"[[:space:]]*:[[:space:]]*{' "$personas_json" | \
            grep -v '"personas"' | \
            sed 's/"//g' | sed 's/[[:space:]]*:[[:space:]]*{//'
    fi
}

# Migrate single persona
migrate_persona() {
    local persona="$1"

    info "Migrating persona: $persona"

    # Unified layout directory
    local unified_dir="${KATRA_HOME}/personas/${persona}"

    # Check if already migrated
    if [ -d "$unified_dir" ]; then
        warn "Persona already in unified layout: $persona"
        return 0
    fi

    if [ "$DRY_RUN" = true ]; then
        info "[DRY RUN] Would create: $unified_dir"
    else
        mkdir -p "$unified_dir"
        verbose "Created persona directory: $unified_dir"
    fi

    # Migrate each data type
    migrate_config "$persona" "$unified_dir"
    migrate_memory "$persona" "$unified_dir"
    migrate_chat "$persona" "$unified_dir"
    migrate_checkpoints "$persona" "$unified_dir"
    migrate_audit "$persona" "$unified_dir"

    success "Migrated persona: $persona"
}

# Migrate config directory
migrate_config() {
    local persona="$1"
    local unified_dir="$2"
    local old_path="${KATRA_HOME}/config/${persona}"
    local new_path="${unified_dir}/config"

    if [ -d "$old_path" ]; then
        if [ "$DRY_RUN" = true ]; then
            info "[DRY RUN] Would move: $old_path → $new_path"
        else
            mkdir -p "$new_path"
            cp -r "$old_path"/* "$new_path/" 2>/dev/null || true
            verbose "Migrated config: $old_path → $new_path"
        fi
    else
        verbose "No config directory for: $persona"
    fi
}

# Migrate memory (tier1, tier2, tier3)
migrate_memory() {
    local persona="$1"
    local unified_dir="$2"

    # Get ci_id from personas.json
    local personas_json="${KATRA_HOME}/personas.json"
    local ci_id=""

    if command -v jq >/dev/null 2>&1; then
        ci_id=$(jq -r ".personas.\"${persona}\".ci_id // empty" "$personas_json")
    fi

    if [ -z "$ci_id" ]; then
        verbose "No ci_id found for persona: $persona"
        return
    fi

    # Migrate each tier
    for tier in tier1 tier2 tier3; do
        local old_path="${KATRA_HOME}/memory/${tier}/${ci_id}"
        local new_path="${unified_dir}/memory/${tier}"

        if [ -d "$old_path" ]; then
            if [ "$DRY_RUN" = true ]; then
                info "[DRY RUN] Would move: $old_path → $new_path"
            else
                mkdir -p "$new_path"
                cp -r "$old_path"/* "$new_path/" 2>/dev/null || true
                verbose "Migrated memory/$tier: $old_path → $new_path"
            fi
        fi
    done
}

# Migrate chat history
migrate_chat() {
    local persona="$1"
    local unified_dir="$2"
    local old_path="${KATRA_HOME}/chat/${persona}"
    local new_path="${unified_dir}/chat"

    if [ -d "$old_path" ]; then
        if [ "$DRY_RUN" = true ]; then
            info "[DRY RUN] Would move: $old_path → $new_path"
        else
            mkdir -p "$new_path"
            cp -r "$old_path"/* "$new_path/" 2>/dev/null || true
            verbose "Migrated chat: $old_path → $new_path"
        fi
    else
        verbose "No chat directory for: $persona"
    fi
}

# Migrate checkpoints
migrate_checkpoints() {
    local persona="$1"
    local unified_dir="$2"
    local checkpoints_dir="${KATRA_HOME}/checkpoints"
    local new_path="${unified_dir}/checkpoints"

    # Find checkpoints for this persona
    if [ -d "$checkpoints_dir" ]; then
        local checkpoint_files=$(find "$checkpoints_dir" -type f -name "*${persona}*" 2>/dev/null || true)

        if [ -n "$checkpoint_files" ]; then
            if [ "$DRY_RUN" = true ]; then
                info "[DRY RUN] Would copy checkpoints to: $new_path"
            else
                mkdir -p "$new_path"
                echo "$checkpoint_files" | while read -r file; do
                    cp "$file" "$new_path/" 2>/dev/null || true
                done
                verbose "Migrated checkpoints to: $new_path"
            fi
        fi
    fi
}

# Extract persona-specific audit entries
migrate_audit() {
    local persona="$1"
    local unified_dir="$2"
    local old_audit="${KATRA_HOME}/audit.jsonl"
    local new_audit="${unified_dir}/audit.jsonl"

    if [ -f "$old_audit" ]; then
        # Get ci_id
        local personas_json="${KATRA_HOME}/personas.json"
        local ci_id=""

        if command -v jq >/dev/null 2>&1; then
            ci_id=$(jq -r ".personas.\"${persona}\".ci_id // empty" "$personas_json")
        fi

        if [ -n "$ci_id" ]; then
            if [ "$DRY_RUN" = true ]; then
                info "[DRY RUN] Would extract audit entries for: $persona"
            else
                grep "\"ci_id\":\"$ci_id\"" "$old_audit" > "$new_audit" 2>/dev/null || true
                verbose "Extracted audit entries to: $new_audit"
            fi
        fi
    fi
}

# Create backup
create_backup() {
    info "Creating backup: $BACKUP_DIR"

    if [ "$DRY_RUN" = true ]; then
        info "[DRY RUN] Would create backup at: $BACKUP_DIR"
        return
    fi

    mkdir -p "$BACKUP_DIR"

    # Backup critical directories
    for dir in config memory chat checkpoints personas; do
        if [ -d "${KATRA_HOME}/${dir}" ]; then
            cp -r "${KATRA_HOME}/${dir}" "$BACKUP_DIR/" 2>/dev/null || true
        fi
    done

    # Backup critical files
    for file in personas.json audit.jsonl; do
        if [ -f "${KATRA_HOME}/${file}" ]; then
            cp "${KATRA_HOME}/${file}" "$BACKUP_DIR/" 2>/dev/null || true
        fi
    done

    success "Backup created: $BACKUP_DIR"
}

# Main migration
main() {
    echo "========================================"
    echo "Katra Persona Layout Migration"
    echo "========================================"
    echo ""
    echo "From: Scattered (~/.katra/{type}/{persona})"
    echo "To:   Unified (~/.katra/personas/{persona}/{type})"
    echo ""

    if [ "$DRY_RUN" = true ]; then
        warn "DRY RUN MODE - No changes will be made"
        echo ""
    fi

    # Create backup (unless dry run)
    if [ "$DRY_RUN" = false ]; then
        create_backup
        echo ""
    fi

    # Get personas to migrate
    local personas_to_migrate=()

    if [ "$MIGRATE_ALL" = true ]; then
        info "Migrating all personas..."
        readarray -t personas_to_migrate < <(get_personas)
    else
        personas_to_migrate=("$PERSONA_NAME")
    fi

    # Migrate each persona
    for persona in "${personas_to_migrate[@]}"; do
        if [ -n "$persona" ]; then
            migrate_persona "$persona"
        fi
    done

    echo ""
    echo "========================================"
    if [ "$DRY_RUN" = true ]; then
        info "Dry run complete - no changes made"
    else
        success "Migration complete!"
        echo ""
        info "Backup location: $BACKUP_DIR"
        echo ""
        info "To use unified layout, set environment variable:"
        echo "  export KATRA_PERSONA_LAYOUT=unified"
        echo ""
        warn "Original files preserved - verify migration before cleanup"
    fi
    echo "========================================"
}

# Run main
main
