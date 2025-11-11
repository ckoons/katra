#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# check_messages.sh - Check for messages and respond
#
# Usage: ./check_messages.sh [persona]
#
# Example:
#   ./check_messages.sh Alice

set -euo pipefail

PERSONA="${1:-${KATRA_PERSONA:-CLI}}"

echo "Checking messages for $PERSONA..."

# Check for messages
MESSAGES=$(k --persona "$PERSONA" --hear)

if [ -n "$MESSAGES" ]; then
    echo ""
    echo "📬 New messages:"
    echo "$MESSAGES"
    echo ""

    # Optional: Auto-respond
    k --persona "$PERSONA" --say "Messages received and acknowledged"
    echo "✅ Acknowledged messages"
else
    echo "📭 No new messages"
fi
