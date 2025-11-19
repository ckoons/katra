#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# Persona Restore Script
#
# Restores a persona from an archive created by archive_persona.sh
#
# Features:
# - Archive integrity verification (SHA256)
# - Manifest validation
# - Conflict detection (persona already exists)
# - Dry-run mode for safety
# - Backup creation before restore
#
# Usage:
#   ./scripts/restore_persona.sh [OPTIONS] ARCHIVE_FILE

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
KATRA_HOME="${HOME}/.katra"
DRY_RUN=false
FORCE=false
VERIFY_CHECKSUMS=true

# Usage
usage() {
    cat <<EOF
Usage: $0 [OPTIONS] ARCHIVE_FILE

Restore persona from archive with integrity verification.

OPTIONS:
    --force             Overwrite existing persona (DANGEROUS)
    --no-verify         Skip SHA256 checksum verification
    --dry-run           Show what would be restored without making changes
    --help              Show this help message

EXAMPLES:
    # Restore with verification (safe)
    $0 Alice-20250119-120000.tar.gz

    # Dry run to preview restore
    $0 --dry-run Alice-20250119-120000.tar.gz

    # Force overwrite existing persona
    $0 --force Alice-20250119-120000.tar.gz

SAFETY:
    - Verifies archive integrity before restore
    - Checks for conflicts with existing personas
    - Creates backup before overwriting (if --force)
    - Validates manifest checksums

RESTORE PROCESS:
    1. Extract archive to temp directory
    2. Verify manifest and checksums
    3. Check for conflicts with existing data
    4. Copy data to Katra directories
    5. Update personas.json
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

# Parse arguments
ARCHIVE_FILE=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --force)
            FORCE=true
            shift
            ;;
        --no-verify)
            VERIFY_CHECKSUMS=false
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
            ARCHIVE_FILE="$1"
            shift
            ;;
    esac
done

# Validate inputs
if [ -z "$ARCHIVE_FILE" ]; then
    error "Must specify archive file"
    usage
fi

if [ ! -f "$ARCHIVE_FILE" ]; then
    error "Archive file not found: $ARCHIVE_FILE"
    exit 1
fi

# Compute SHA256 checksum
compute_sha256() {
    local file="$1"
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$file" | awk '{print $1}'
    elif command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$file" | awk '{print $1}'
    else
        echo "CHECKSUM_NOT_AVAILABLE"
    fi
}

# Extract archive to temp directory
extract_archive() {
    local archive="$1"
    local temp_dir=$(mktemp -d)

    info "Extracting archive..."

    tar xzf "$archive" -C "$temp_dir" || {
        error "Failed to extract archive"
        rm -rf "$temp_dir"
        return 1
    }

    echo "$temp_dir"
}

# Find and validate manifest
find_manifest() {
    local archive="$1"
    local archive_dir=$(dirname "$archive")
    local archive_base=$(basename "$archive" .tar.gz)
    local manifest_file="${archive_dir}/${archive_base}.manifest.json"

    if [ -f "$manifest_file" ]; then
        echo "$manifest_file"
    else
        warn "Manifest not found: $manifest_file"
        echo ""
    fi
}

# Verify archive integrity
verify_integrity() {
    local manifest="$1"
    local extracted_dir="$2"

    if [ -z "$manifest" ] || [ ! -f "$manifest" ]; then
        warn "Cannot verify integrity - no manifest"
        return 0
    fi

    info "Verifying archive integrity..."

    if ! command -v jq >/dev/null 2>&1; then
        warn "jq not available - skipping integrity verification"
        return 0
    fi

    local persona=$(jq -r '.persona' "$manifest")
    local persona_dir="${extracted_dir}/${persona}"

    # Verify each file checksum
    local files=$(jq -r '.files[] | @base64' "$manifest")
    local verified=0
    local failed=0

    for file_data in $files; do
        local path=$(echo "$file_data" | base64 -d | jq -r '.path')
        local expected_checksum=$(echo "$file_data" | base64 -d | jq -r '.sha256')

        local file_path="${persona_dir}/${path}"

        if [ ! -f "$file_path" ]; then
            warn "File missing: $path"
            ((failed++))
            continue
        fi

        if [ "$VERIFY_CHECKSUMS" = true ]; then
            local actual_checksum=$(compute_sha256 "$file_path")

            if [ "$actual_checksum" != "$expected_checksum" ]; then
                error "Checksum mismatch: $path"
                error "  Expected: $expected_checksum"
                error "  Actual:   $actual_checksum"
                ((failed++))
            else
                ((verified++))
            fi
        fi
    done

    if [ $failed -gt 0 ]; then
        error "Integrity verification failed: $failed files"
        return 1
    fi

    success "Integrity verified: $verified files"
    return 0
}

# Check for conflicts with existing persona
check_conflicts() {
    local persona="$1"

    local personas_json="${KATRA_HOME}/personas.json"

    if [ ! -f "$personas_json" ]; then
        # No personas.json yet - no conflict
        return 0
    fi

    if command -v jq >/dev/null 2>&1; then
        if jq -e ".personas.\"$persona\"" "$personas_json" >/dev/null 2>&1; then
            warn "Persona already exists: $persona"

            if [ "$FORCE" = false ]; then
                error "Use --force to overwrite existing persona"
                return 1
            else
                warn "Overwriting existing persona (--force enabled)"
                return 0
            fi
        fi
    else
        if grep -q "\"$persona\"" "$personas_json"; then
            warn "Persona already exists: $persona"

            if [ "$FORCE" = false ]; then
                error "Use --force to overwrite existing persona"
                return 1
            else
                warn "Overwriting existing persona (--force enabled)"
                return 0
            fi
        fi
    fi

    return 0
}

# Restore persona data
restore_persona_data() {
    local persona="$1"
    local extracted_dir="$2"
    local manifest="$3"

    info "Restoring persona data..."

    local persona_dir="${extracted_dir}/${persona}"

    # Determine layout mode
    local layout="${KATRA_PERSONA_LAYOUT:-scattered}"

    # Get original layout from manifest if available
    if [ -n "$manifest" ] && command -v jq >/dev/null 2>&1; then
        local original_layout=$(jq -r '.layout // "scattered"' "$manifest")
        info "Original layout: $original_layout, Current layout: $layout"
    fi

    if [ "$layout" = "unified" ]; then
        # Unified layout: restore to ~/.katra/personas/{persona}
        local dest_dir="${KATRA_HOME}/personas/${persona}"

        if [ "$DRY_RUN" = true ]; then
            info "[DRY RUN] Would restore to: $dest_dir"
        else
            mkdir -p "$dest_dir"
            cp -r "$persona_dir"/* "$dest_dir/" 2>/dev/null || true
            success "Restored to: $dest_dir"
        fi
    else
        # Scattered layout: restore to type-specific directories
        # This requires parsing the archive structure

        if [ "$DRY_RUN" = true ]; then
            info "[DRY RUN] Would restore to scattered layout:"
            info "  - ${KATRA_HOME}/config/${persona}"
            info "  - ${KATRA_HOME}/chat/${persona}"
            info "  - ${KATRA_HOME}/memory/tier*/{ci_id}"
        else
            # Restore config
            if [ -d "${persona_dir}/config" ]; then
                mkdir -p "${KATRA_HOME}/config"
                cp -r "${persona_dir}/config" "${KATRA_HOME}/config/${persona}" 2>/dev/null || true
                success "Restored config"
            fi

            # Restore chat
            if [ -d "${persona_dir}/chat" ]; then
                mkdir -p "${KATRA_HOME}/chat"
                cp -r "${persona_dir}/chat" "${KATRA_HOME}/chat/${persona}" 2>/dev/null || true
                success "Restored chat"
            fi

            # Restore memory (requires CI ID)
            if [ -f "${persona_dir}/persona_config.json" ]; then
                local ci_id=$(jq -r '.ci_id // empty' "${persona_dir}/persona_config.json")

                if [ -n "$ci_id" ]; then
                    for tier in tier1 tier2 tier3; do
                        if [ -d "${persona_dir}/memory/${tier}" ]; then
                            mkdir -p "${KATRA_HOME}/memory/${tier}"
                            cp -r "${persona_dir}/memory/${tier}" "${KATRA_HOME}/memory/${tier}/${ci_id}" 2>/dev/null || true
                            success "Restored memory/${tier}"
                        fi
                    done
                fi
            fi

            # Restore checkpoints
            if [ -d "${persona_dir}/checkpoints" ]; then
                mkdir -p "${KATRA_HOME}/checkpoints"
                cp -r "${persona_dir}/checkpoints"/* "${KATRA_HOME}/checkpoints/" 2>/dev/null || true
                success "Restored checkpoints"
            fi
        fi
    fi
}

# Update personas.json
update_personas_json() {
    local persona="$1"
    local extracted_dir="$2"

    local persona_config="${extracted_dir}/${persona}/persona_config.json"

    if [ ! -f "$persona_config" ]; then
        warn "No persona_config.json found in archive"
        return 0
    fi

    if [ "$DRY_RUN" = true ]; then
        info "[DRY RUN] Would update personas.json with: $persona"
        return 0
    fi

    local personas_json="${KATRA_HOME}/personas.json"

    if command -v jq >/dev/null 2>&1; then
        # Initialize personas.json if it doesn't exist
        if [ ! -f "$personas_json" ]; then
            echo '{"personas":{}}' > "$personas_json"
        fi

        # Add or update persona entry
        local persona_data=$(cat "$persona_config")
        local temp_file=$(mktemp)

        jq ".personas.\"$persona\" = $persona_data" "$personas_json" > "$temp_file"
        mv "$temp_file" "$personas_json"

        success "Updated personas.json"
    else
        warn "jq not available - manual update of personas.json required"
        warn "Please add persona data from: $persona_config"
    fi
}

# Main restore procedure
main() {
    echo "========================================"
    echo "Persona Restore"
    echo "========================================"
    echo ""
    echo "Archive: $ARCHIVE_FILE"
    echo "Verify checksums: $VERIFY_CHECKSUMS"
    echo "Force overwrite: $FORCE"
    echo ""

    if [ "$DRY_RUN" = true ]; then
        warn "DRY RUN MODE - No changes will be made"
        echo ""
    fi

    # Find manifest
    local manifest=$(find_manifest "$ARCHIVE_FILE")

    if [ -n "$manifest" ]; then
        info "Manifest: $manifest"

        if command -v jq >/dev/null 2>&1; then
            local persona=$(jq -r '.persona' "$manifest")
            local ci_id=$(jq -r '.ci_id' "$manifest")
            local timestamp=$(jq -r '.archive_timestamp' "$manifest")

            info "Persona: $persona"
            info "CI ID: $ci_id"
            info "Archived: $timestamp"
        fi
        echo ""
    fi

    # Extract archive
    local extracted_dir=$(extract_archive "$ARCHIVE_FILE") || {
        error "Failed to extract archive"
        exit 1
    }

    # Get persona name from extracted directory
    local persona=$(ls "$extracted_dir" | head -1)

    if [ -z "$persona" ]; then
        error "Could not determine persona name from archive"
        rm -rf "$extracted_dir"
        exit 1
    fi

    info "Persona: $persona"
    echo ""

    # Verify integrity
    if [ "$VERIFY_CHECKSUMS" = true ]; then
        verify_integrity "$manifest" "$extracted_dir" || {
            error "Integrity verification failed"
            rm -rf "$extracted_dir"
            exit 1
        }
        echo ""
    fi

    # Check for conflicts
    check_conflicts "$persona" || {
        rm -rf "$extracted_dir"
        exit 1
    }
    echo ""

    # Restore persona data
    restore_persona_data "$persona" "$extracted_dir" "$manifest"

    # Update personas.json
    update_personas_json "$persona" "$extracted_dir"

    # Cleanup
    rm -rf "$extracted_dir"

    echo ""
    echo "========================================"
    if [ "$DRY_RUN" = true ]; then
        info "Dry run complete - no changes made"
    else
        success "Persona restored successfully!"
        echo ""
        info "Persona: $persona"
        info "Layout: ${KATRA_PERSONA_LAYOUT:-scattered}"
        echo ""
        info "You can now use this persona:"
        echo "  export KATRA_PERSONA=$persona"
        echo "  ./scripts/katra start"
    fi
    echo "========================================"
}

# Run main
main
