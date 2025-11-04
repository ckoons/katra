#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# Katra MCP Server Wrapper Script
#
# Purpose: Ensures MCP server runs in correct working directory
# so that .env.katra configuration is properly loaded.

# Get the directory containing this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Katra project root is one level up from bin/
KATRA_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Change to Katra root directory (required for .env.katra loading)
cd "$KATRA_ROOT" || exit 1

# Execute MCP server binary (in bin/ directory)
exec "$KATRA_ROOT/bin/katra_mcp_server"
