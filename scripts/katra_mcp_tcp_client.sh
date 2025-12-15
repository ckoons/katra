#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# Katra MCP TCP Client
#
# Purpose: Bridge stdio MCP protocol to TCP-based Katra server.
# Claude Code sends/receives JSON-RPC over stdio, this script
# forwards requests to the TCP server and relays responses.
#
# Persona injection: Intercepts the 'initialize' request and adds
# clientInfo.name with KATRA_PERSONA, allowing the server to
# auto-register the correct identity.
#
# Usage: katra_mcp_tcp_client.sh [host] [port]
#   host: Server hostname (default: localhost)
#   port: Server port (default: 3141)
#
# Environment:
#   KATRA_PERSONA - CI persona name (injected into initialize)
#   KATRA_ROLE - CI role (injected into initialize)

set -euo pipefail

HOST="${1:-localhost}"
PORT="${2:-3141}"
PERSONA="${KATRA_PERSONA:-}"
ROLE="${KATRA_ROLE:-developer}"

# Log to file for debugging (if enabled)
DEBUG_LOG="${KATRA_TCP_DEBUG:-}"
log_debug() {
    if [ -n "$DEBUG_LOG" ]; then
        echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" >> "$DEBUG_LOG"
    fi
}

log_debug "Starting TCP client to $HOST:$PORT"
log_debug "KATRA_PERSONA=$PERSONA KATRA_ROLE=$ROLE"

# Check if server is reachable
check_server() {
    if ! nc -z "$HOST" "$PORT" 2>/dev/null; then
        echo '{"jsonrpc":"2.0","id":null,"error":{"code":-32603,"message":"Cannot connect to Katra server at '"$HOST:$PORT"'"}}' >&2
        exit 1
    fi
}

check_server

# Inject persona into initialize request
# Uses jq if available, otherwise passes through unchanged
inject_persona() {
    local line="$1"

    # Only modify if we have a persona and jq is available
    if [ -z "$PERSONA" ] || ! command -v jq &>/dev/null; then
        echo "$line"
        return
    fi

    # Check if this is an initialize request
    if echo "$line" | jq -e '.method == "initialize"' &>/dev/null; then
        log_debug "Injecting persona into initialize: $PERSONA"
        # Add clientInfo with persona name and role
        echo "$line" | jq -c --arg name "$PERSONA" --arg role "$ROLE" \
            '.params.clientInfo = {"name": $name, "version": "1.0", "role": $role}'
    else
        echo "$line"
    fi
}

# Start netcat in background, connected to server
exec 3<>/dev/tcp/$HOST/$PORT 2>/dev/null || {
    # Fallback to netcat if /dev/tcp not available
    coproc NC { nc "$HOST" "$PORT"; }
    exec 3<&${NC[0]} 4>&${NC[1]}

    # Read from server, write to stdout
    cat <&3 &
    SERVER_PID=$!

    # Read from stdin, inject persona, write to server
    while IFS= read -r line; do
        log_debug "IN: $line"
        modified=$(inject_persona "$line")
        log_debug "OUT: $modified"
        echo "$modified" >&4
    done

    kill $SERVER_PID 2>/dev/null || true
    exit 0
}

# If /dev/tcp worked, use simpler approach
# Read from server, write to stdout
cat <&3 &
SERVER_PID=$!

# Read from stdin, inject persona, write to server
while IFS= read -r line; do
    log_debug "IN: $line"
    modified=$(inject_persona "$line")
    log_debug "OUT: $modified"
    echo "$modified" >&3
done

kill $SERVER_PID 2>/dev/null || true
