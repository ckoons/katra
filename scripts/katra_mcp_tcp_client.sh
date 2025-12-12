#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# Katra MCP TCP Client
#
# Purpose: Bridge stdio MCP protocol to TCP-based Katra server.
# Claude Code sends/receives JSON-RPC over stdio, this script
# forwards requests to the TCP server and relays responses.
#
# Usage: katra_mcp_tcp_client.sh [host] [port]
#   host: Server hostname (default: localhost)
#   port: Server port (default: 3141)
#
# Environment:
#   KATRA_PERSONA - CI persona name (passed to server)
#   KATRA_ROLE - CI role (passed to server)

set -euo pipefail

HOST="${1:-localhost}"
PORT="${2:-3141}"

# Log to file for debugging (if enabled)
DEBUG_LOG="${KATRA_TCP_DEBUG:-}"
log_debug() {
    if [ -n "$DEBUG_LOG" ]; then
        echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" >> "$DEBUG_LOG"
    fi
}

log_debug "Starting TCP client to $HOST:$PORT"
log_debug "KATRA_PERSONA=${KATRA_PERSONA:-} KATRA_ROLE=${KATRA_ROLE:-}"

# Check if server is reachable
check_server() {
    if ! nc -z "$HOST" "$PORT" 2>/dev/null; then
        echo '{"jsonrpc":"2.0","id":null,"error":{"code":-32603,"message":"Cannot connect to Katra server at '"$HOST:$PORT"'"}}' >&2
        exit 1
    fi
}

check_server

# Use netcat to establish bidirectional communication
# This reads from stdin, sends to TCP, and writes responses to stdout
exec nc "$HOST" "$PORT"
