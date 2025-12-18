#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# TCP bridge for MCP protocol with auto-start
# Bridges stdio (Claude Code) <-> TCP (katra_unified_daemon)
#
# Usage: katra_mcp_tcp_client.sh [host] [port]
#
# If daemon is not running, starts it automatically.

HOST="${1:-localhost}"
PORT="${2:-3141}"

# Get script directory to find daemon
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KATRA_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
DAEMON="$KATRA_ROOT/bin/katra_unified_daemon"

# Check if daemon is running by testing connection
if ! nc -z "$HOST" "$PORT" 2>/dev/null; then
    # Daemon not running - start it
    if [ -x "$DAEMON" ]; then
        cd "$KATRA_ROOT" || exit 1
        "$DAEMON" --mcp-port "$PORT" --port 9742 >/dev/null 2>&1 &
        # Wait for daemon to start (up to 3 seconds)
        for i in 1 2 3 4 5 6; do
            sleep 0.5
            if nc -z "$HOST" "$PORT" 2>/dev/null; then
                break
            fi
        done
    fi
fi

# Connect to daemon
exec nc "$HOST" "$PORT"
