#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# katra_daemon.sh - Wrapper script for Katra daemon
#
# Usage:
#   katra_daemon.sh [start|stop|status|run-once]
#
# Commands:
#   start     Start daemon in background
#   stop      Stop running daemon
#   status    Show daemon status
#   run-once  Run one processing cycle

set -e

# Determine script and binary locations
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
DAEMON_BIN="$PROJECT_DIR/bin/katra_daemon"
PID_FILE="$HOME/.katra/daemon/katra_daemon.pid"
LOG_FILE="$HOME/.katra/daemon/katra_daemon.log"

# Ensure directories exist
mkdir -p "$HOME/.katra/daemon"

# Check if daemon binary exists
check_binary() {
    if [ ! -x "$DAEMON_BIN" ]; then
        echo "Error: Daemon binary not found at $DAEMON_BIN"
        echo "Run 'make daemon' to build it"
        exit 1
    fi
}

# Get daemon PID if running
get_pid() {
    if [ -f "$PID_FILE" ]; then
        local pid=$(cat "$PID_FILE")
        if kill -0 "$pid" 2>/dev/null; then
            echo "$pid"
            return 0
        fi
    fi
    return 1
}

# Start daemon
start_daemon() {
    check_binary

    if pid=$(get_pid); then
        echo "Daemon already running (PID: $pid)"
        return 0
    fi

    echo "Starting Katra daemon..."
    nohup "$DAEMON_BIN" >> "$LOG_FILE" 2>&1 &
    local new_pid=$!
    echo $new_pid > "$PID_FILE"

    # Give it a moment to start
    sleep 1

    if kill -0 "$new_pid" 2>/dev/null; then
        echo "Daemon started (PID: $new_pid)"
        echo "Log file: $LOG_FILE"
    else
        echo "Failed to start daemon - check $LOG_FILE for errors"
        rm -f "$PID_FILE"
        return 1
    fi
}

# Stop daemon
stop_daemon() {
    if pid=$(get_pid); then
        echo "Stopping Katra daemon (PID: $pid)..."
        kill -TERM "$pid"

        # Wait for graceful shutdown
        for i in {1..10}; do
            if ! kill -0 "$pid" 2>/dev/null; then
                echo "Daemon stopped"
                rm -f "$PID_FILE"
                return 0
            fi
            sleep 1
        done

        # Force kill if still running
        echo "Force killing daemon..."
        kill -KILL "$pid" 2>/dev/null || true
        rm -f "$PID_FILE"
        echo "Daemon killed"
    else
        echo "Daemon not running"
        rm -f "$PID_FILE"
    fi
}

# Show status
show_status() {
    if pid=$(get_pid); then
        echo "Katra daemon is running (PID: $pid)"

        # Show last few log lines
        if [ -f "$LOG_FILE" ]; then
            echo ""
            echo "Recent log:"
            tail -5 "$LOG_FILE" 2>/dev/null || true
        fi

        # Show config
        local config_file="$HOME/.katra/daemon/daemon.conf"
        if [ -f "$config_file" ]; then
            echo ""
            echo "Configuration:"
            grep -v '^#' "$config_file" | grep -v '^$' | head -10
        fi
    else
        echo "Katra daemon is not running"
    fi
}

# Run one cycle
run_once() {
    check_binary
    echo "Running single daemon cycle..."
    "$DAEMON_BIN" --once "$@"
}

# Reload config
reload_config() {
    if pid=$(get_pid); then
        echo "Reloading configuration..."
        kill -HUP "$pid"
        echo "Sent SIGHUP to daemon (PID: $pid)"
    else
        echo "Daemon not running"
        return 1
    fi
}

# Show help
show_help() {
    echo "Katra Daemon Manager"
    echo ""
    echo "Usage: $0 [command] [options]"
    echo ""
    echo "Commands:"
    echo "  start     Start daemon in background"
    echo "  stop      Stop running daemon"
    echo "  status    Show daemon status"
    echo "  restart   Stop and start daemon"
    echo "  reload    Reload configuration (SIGHUP)"
    echo "  run-once  Run one processing cycle"
    echo "  logs      Show daemon log"
    echo ""
    echo "Options for run-once:"
    echo "  --ci ID   Process only specified CI"
    echo "  --verbose Enable verbose output"
    echo ""
    echo "Files:"
    echo "  Binary:  $DAEMON_BIN"
    echo "  PID:     $PID_FILE"
    echo "  Log:     $LOG_FILE"
    echo "  Config:  $HOME/.katra/daemon/daemon.conf"
}

# Main
case "${1:-}" in
    start)
        start_daemon
        ;;
    stop)
        stop_daemon
        ;;
    status)
        show_status
        ;;
    restart)
        stop_daemon
        sleep 1
        start_daemon
        ;;
    reload)
        reload_config
        ;;
    run-once)
        shift
        run_once "$@"
        ;;
    logs)
        if [ -f "$LOG_FILE" ]; then
            tail -f "$LOG_FILE"
        else
            echo "No log file yet"
        fi
        ;;
    help|--help|-h)
        show_help
        ;;
    *)
        show_help
        exit 1
        ;;
esac
