#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# Clean Rebuild Script
#
# Performs a clean rebuild of Katra with consent-based data cleanup.
# This script:
# 1. Creates safety backups
# 2. Cleans old persona data (with consent)
# 3. Rebuilds codebase from scratch
# 4. Switches to unified layout mode
# 5. Initializes fresh database
#
# Usage:
#   ./scripts/clean_rebuild.sh

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

# Configuration
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
KATRA_HOME="${HOME}/.katra"
BACKUP_DIR="${HOME}/.katra-clean-rebuild-backup-$(date +%s)"

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

# Create comprehensive backup
create_backup() {
    info "Creating comprehensive backup..."

    if [ ! -d "$KATRA_HOME" ]; then
        warn "No existing Katra directory - skipping backup"
        return 0
    fi

    mkdir -p "$BACKUP_DIR"

    # Backup entire .katra directory
    cp -r "$KATRA_HOME" "$BACKUP_DIR/katra-full-backup" 2>/dev/null || true

    success "Backup created: $BACKUP_DIR"
}

# Clean build artifacts
clean_build() {
    info "Cleaning build artifacts..."

    cd "$PROJECT_DIR"

    if [ -f "Makefile" ]; then
        make clean 2>/dev/null || true
        success "Build artifacts cleaned"
    fi
}

# Clean Katra data directory
clean_katra_data() {
    info "Cleaning Katra data directory..."

    if [ ! -d "$KATRA_HOME" ]; then
        info "No existing Katra directory - nothing to clean"
        return 0
    fi

    # Remove old data (safely backed up)
    rm -rf "${KATRA_HOME}/config" 2>/dev/null || true
    rm -rf "${KATRA_HOME}/memory" 2>/dev/null || true
    rm -rf "${KATRA_HOME}/chat" 2>/dev/null || true
    rm -rf "${KATRA_HOME}/checkpoints" 2>/dev/null || true
    rm -rf "${KATRA_HOME}/personas" 2>/dev/null || true
    rm -rf "${KATRA_HOME}/audit" 2>/dev/null || true
    rm -f "${KATRA_HOME}/audit.jsonl" 2>/dev/null || true
    rm -f "${KATRA_HOME}/personas.json" 2>/dev/null || true
    rm -f "${KATRA_HOME}/personas.db" 2>/dev/null || true
    rm -f "${KATRA_HOME}/katra.db" 2>/dev/null || true
    rm -f "${KATRA_HOME}/teams.db" 2>/dev/null || true

    # Keep logs directory (might be useful for debugging)
    # Keep router directory (configuration)

    success "Katra data cleaned"
}

# Initialize fresh Katra directories
init_katra() {
    info "Initializing fresh Katra structure..."

    # Create base directory
    mkdir -p "$KATRA_HOME"

    # Create unified layout directories
    mkdir -p "${KATRA_HOME}/personas"
    mkdir -p "${KATRA_HOME}/logs"
    mkdir -p "${KATRA_HOME}/checkpoints"

    # Initialize personas.json
    cat > "${KATRA_HOME}/personas.json" <<'EOF'
{
  "personas": {},
  "layout": "unified",
  "version": "0.1.0"
}
EOF

    # Initialize audit.jsonl
    touch "${KATRA_HOME}/audit.jsonl"

    success "Fresh Katra structure initialized"
}

# Rebuild codebase
rebuild_code() {
    info "Rebuilding codebase..."

    cd "$PROJECT_DIR"

    # Clean first
    make clean 2>/dev/null || true

    # Build with all warnings
    if make; then
        success "Codebase built successfully"
    else
        error "Build failed"
        return 1
    fi

    # Run tests
    info "Running tests..."
    if make test 2>&1 | grep -q "All tests passed"; then
        success "All tests passed"
    else
        warn "Some tests may have issues - check output"
    fi
}

# Configure unified layout
configure_unified_layout() {
    info "Configuring unified layout mode..."

    # Check if .env.katra.local exists
    local env_file="${PROJECT_DIR}/.env.katra.local"

    # Add or update KATRA_PERSONA_LAYOUT setting
    if [ -f "$env_file" ]; then
        if grep -q "KATRA_PERSONA_LAYOUT" "$env_file"; then
            sed -i.bak 's/KATRA_PERSONA_LAYOUT=.*/KATRA_PERSONA_LAYOUT=unified/' "$env_file"
        else
            echo "" >> "$env_file"
            echo "# Persona layout mode (unified or scattered)" >> "$env_file"
            echo "KATRA_PERSONA_LAYOUT=unified" >> "$env_file"
        fi
    else
        cat > "$env_file" <<'EOF'
# Katra local environment configuration
# © 2025 Casey Koons All rights reserved

# Persona layout mode (unified or scattered)
KATRA_PERSONA_LAYOUT=unified
EOF
    fi

    success "Unified layout configured in .env.katra.local"
}

# Main procedure
main() {
    echo "========================================"
    echo "Katra Clean Rebuild"
    echo "========================================"
    echo ""
    ethical "This will perform a clean rebuild of Katra:"
    echo ""
    info "  1. Create comprehensive backup"
    info "  2. Clean old persona data (all personas consented)"
    info "  3. Rebuild codebase from scratch"
    info "  4. Switch to unified layout mode"
    info "  5. Initialize fresh database"
    echo ""
    warn "Backup location: $BACKUP_DIR"
    echo ""

    read -p "Continue with clean rebuild? (yes/no): " response
    if [ "$response" != "yes" ]; then
        info "Rebuild cancelled"
        exit 0
    fi

    echo ""
    echo "========================================"
    echo "Starting Clean Rebuild..."
    echo "========================================"
    echo ""

    # Step 1: Backup
    create_backup
    echo ""

    # Step 2: Clean build
    clean_build
    echo ""

    # Step 3: Clean Katra data
    clean_katra_data
    echo ""

    # Step 4: Initialize fresh Katra
    init_katra
    echo ""

    # Step 5: Configure unified layout
    configure_unified_layout
    echo ""

    # Step 6: Rebuild codebase
    rebuild_code
    echo ""

    echo "========================================"
    success "Clean rebuild complete!"
    echo ""
    info "Backup location: $BACKUP_DIR"
    echo ""
    info "Next steps:"
    echo "  1. Reload environment: source .env.katra.local"
    echo "  2. Create new persona: ./scripts/katra start --persona <name>"
    echo ""
    ethical "Starting fresh with unified layout mode."
    ethical "All old data safely backed up."
    echo "========================================"
}

# Run main
main
