#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# Simple TCP bridge for MCP protocol
# Bridges stdio (Claude Code) <-> TCP (katra_unified_daemon)
#
# Usage: katra_mcp_tcp_client.sh [host] [port]
#
# NOTE: Keep this simple! Complex bash with /dev/tcp, coproc, etc.
# fails on macOS. Plain netcat works reliably.

exec nc "${1:-localhost}" "${2:-3141}"
