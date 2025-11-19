#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# Persona Archive Script
#
# Creates a complete, portable archive of a persona's data.
# Archives include all memory tiers, config, chat history, and checkpoints.
#
# Features:
# - Complete persona state capture
# - SHA256 integrity verification
# - Manifest generation with metadata
# - Compression for portability
# - Optional encryption support
#
# Usage:
#   ./scripts/archive_persona.sh [OPTIONS] PERSONA_NAME

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
KATRA_HOME="${HOME}/.katra"
ARCHIVE_DIR="${HOME}/.katra/archives"
DRY_RUN=false
INCLUDE_CHECKPOINTS=true
COMPRESS=true

# Usage
usage() {
    cat <<EOF
Usage: $0 [OPTIONS] PERSONA_NAME

Create a portable archive of persona data.

OPTIONS:
    --output DIR        Archive output directory (default: ~/.katra/archives)
    --no-checkpoints    Exclude checkpoints from archive
    --no-compress       Do not compress archive (faster, larger)
    --dry-run           Show what would be archived without creating archive
    --help              Show this help message

EXAMPLES:
    # Create archive with all data
    $0 Alice

    # Archive without checkpoints (smaller)
    $0 --no-checkpoints Alice

    # Archive to custom location
    $0 --output /backup/katra Alice

ARCHIVE CONTENTS:
    - Configuration files
    - Memory (tier1, tier2, tier3)
    - Chat history
    - Checkpoints (optional)
    - Audit log entries
    - Manifest with SHA256 checksums

OUTPUT:
    Archive file: {output_dir}/{persona}-{timestamp}.tar.gz
    Manifest file: {output_dir}/{persona}-{timestamp}.manifest.json
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
PERSONA_NAME=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --output)
            ARCHIVE_DIR="$2"
            shift 2
            ;;
        --no-checkpoints)
            INCLUDE_CHECKPOINTS=false
            shift
            ;;
        --no-compress)
            COMPRESS=false
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

# Verify persona exists
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

# Get persona paths based on layout
get_persona_paths() {
    local persona="$1"
    local ci_id="$2"

    if [ "$LAYOUT" = "unified" ]; then
        # Unified layout: ~/.katra/personas/{persona}/*
        echo "${KATRA_HOME}/personas/${persona}"
    else
        # Scattered layout: list all paths
        echo "${KATRA_HOME}/config/${persona}"
        echo "${KATRA_HOME}/chat/${persona}"
        echo "${KATRA_HOME}/memory/tier1/${ci_id}"
        echo "${KATRA_HOME}/memory/tier2/${ci_id}"
        echo "${KATRA_HOME}/memory/tier3/${ci_id}"
    fi
}

# Calculate directory size
calculate_size() {
    local path="$1"
    if [ -d "$path" ]; then
        du -sh "$path" 2>/dev/null | awk '{print $1}'
    else
        echo "0B"
    fi
}

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

# Create archive staging directory
create_staging_dir() {
    local temp_dir=$(mktemp -d)
    echo "$temp_dir"
}

# Copy persona data to staging
stage_persona_data() {
    local persona="$1"
    local ci_id="$2"
    local staging_dir="$3"

    info "Staging persona data..."

    # Create persona directory in staging
    local persona_staging="${staging_dir}/${persona}"
    mkdir -p "$persona_staging"

    # Get all paths to archive
    local paths=$(get_persona_paths "$persona" "$ci_id")

    for path in $paths; do
        if [ -d "$path" ]; then
            local basename=$(basename "$path")
            local dirname=$(dirname "$path" | xargs basename)

            # Preserve directory structure
            if [ "$LAYOUT" = "scattered" ]; then
                mkdir -p "${persona_staging}/${dirname}"
                cp -r "$path" "${persona_staging}/${dirname}/" 2>/dev/null || true
            else
                cp -r "$path"/* "${persona_staging}/" 2>/dev/null || true
            fi

            success "Staged: $path ($(calculate_size "$path"))"
        fi
    done

    # Copy checkpoints if requested
    if [ "$INCLUDE_CHECKPOINTS" = true ]; then
        local checkpoints_dir="${KATRA_HOME}/checkpoints"
        if [ -d "$checkpoints_dir" ]; then
            local checkpoint_files=$(find "$checkpoints_dir" -type f -name "*${persona}*" 2>/dev/null || true)
            if [ -n "$checkpoint_files" ]; then
                mkdir -p "${persona_staging}/checkpoints"
                echo "$checkpoint_files" | while read -r file; do
                    cp "$file" "${persona_staging}/checkpoints/" 2>/dev/null || true
                done
                success "Staged checkpoints: $(calculate_size "${persona_staging}/checkpoints")"
            fi
        fi
    fi

    # Extract persona-specific audit entries
    local audit_file="${KATRA_HOME}/audit.jsonl"
    if [ -f "$audit_file" ]; then
        grep "\"ci_id\":\"$ci_id\"" "$audit_file" > "${persona_staging}/audit.jsonl" 2>/dev/null || true
        success "Staged audit entries"
    fi

    # Copy persona config from personas.json
    if command -v jq >/dev/null 2>&1; then
        jq ".personas.\"$persona\"" "$PERSONAS_JSON" > "${persona_staging}/persona_config.json"
        success "Staged persona configuration"
    fi
}

# Generate manifest
generate_manifest() {
    local persona="$1"
    local staging_dir="$2"
    local manifest_file="$3"

    info "Generating manifest..."

    local timestamp=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
    local total_size=$(calculate_size "${staging_dir}/${persona}")

    # Start manifest
    cat > "$manifest_file" <<EOF
{
  "persona": "$persona",
  "ci_id": "$CI_ID",
  "archive_timestamp": "$timestamp",
  "layout": "$LAYOUT",
  "total_size": "$total_size",
  "includes_checkpoints": $INCLUDE_CHECKPOINTS,
  "files": [
EOF

    # Generate file list with checksums
    local first=true
    find "${staging_dir}/${persona}" -type f | while read -r file; do
        local rel_path=$(echo "$file" | sed "s|${staging_dir}/${persona}/||")
        local file_size=$(du -h "$file" | awk '{print $1}')
        local checksum=$(compute_sha256 "$file")

        if [ "$first" = true ]; then
            first=false
        else
            echo "," >> "$manifest_file"
        fi

        cat >> "$manifest_file" <<EOF
    {
      "path": "$rel_path",
      "size": "$file_size",
      "sha256": "$checksum"
    }
EOF
    done

    # Close manifest
    cat >> "$manifest_file" <<EOF

  ],
  "katra_version": "0.1.0",
  "archive_format": "tar.gz"
}
EOF

    success "Manifest generated: $manifest_file"
}

# Create archive
create_archive() {
    local persona="$1"
    local staging_dir="$2"
    local archive_file="$3"

    info "Creating archive..."

    if [ "$COMPRESS" = true ]; then
        (cd "$staging_dir" && tar czf "$archive_file" "$persona") || {
            error "Failed to create archive"
            return 1
        }
    else
        (cd "$staging_dir" && tar cf "$archive_file" "$persona") || {
            error "Failed to create archive"
            return 1
        }
    fi

    local archive_size=$(du -h "$archive_file" | awk '{print $1}')
    success "Archive created: $archive_file ($archive_size)"
}

# Main archive procedure
main() {
    echo "========================================"
    echo "Persona Archive"
    echo "========================================"
    echo ""
    echo "Persona: $PERSONA_NAME"
    echo "CI ID: $CI_ID"
    echo "Layout: $LAYOUT"
    echo "Include checkpoints: $INCLUDE_CHECKPOINTS"
    echo "Compress: $COMPRESS"
    echo ""

    if [ "$DRY_RUN" = true ]; then
        warn "DRY RUN MODE - No archive will be created"
        echo ""

        info "Would archive the following:"
        local paths=$(get_persona_paths "$PERSONA_NAME" "$CI_ID")
        for path in $paths; do
            if [ -d "$path" ] || [ -f "$path" ]; then
                echo "  - $path ($(calculate_size "$path"))"
            fi
        done

        if [ "$INCLUDE_CHECKPOINTS" = true ]; then
            local checkpoints_dir="${KATRA_HOME}/checkpoints"
            if [ -d "$checkpoints_dir" ]; then
                local checkpoint_count=$(find "$checkpoints_dir" -type f -name "*${PERSONA_NAME}*" 2>/dev/null | wc -l)
                echo "  - Checkpoints: $checkpoint_count files"
            fi
        fi

        echo ""
        info "Dry run complete"
        return 0
    fi

    # Create archive directory
    mkdir -p "$ARCHIVE_DIR"

    # Generate archive filename
    local timestamp=$(date +%Y%m%d-%H%M%S)
    local archive_file="${ARCHIVE_DIR}/${PERSONA_NAME}-${timestamp}.tar.gz"
    local manifest_file="${ARCHIVE_DIR}/${PERSONA_NAME}-${timestamp}.manifest.json"

    # Create staging directory
    local staging_dir=$(create_staging_dir)

    # Stage persona data
    stage_persona_data "$PERSONA_NAME" "$CI_ID" "$staging_dir"

    # Generate manifest
    generate_manifest "$PERSONA_NAME" "$staging_dir" "$manifest_file"

    # Create archive
    create_archive "$PERSONA_NAME" "$staging_dir" "$archive_file"

    # Cleanup staging
    rm -rf "$staging_dir"

    # Generate archive checksum
    local archive_checksum=$(compute_sha256 "$archive_file")

    echo ""
    echo "========================================"
    success "Archive complete!"
    echo ""
    info "Archive: $archive_file"
    info "Manifest: $manifest_file"
    info "SHA256: $archive_checksum"
    echo ""
    info "To restore this archive:"
    echo "  ./scripts/restore_persona.sh $archive_file"
    echo "========================================"
}

# Run main
main
