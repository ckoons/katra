#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# daily_standup.sh - Generate daily standup summary
#
# Usage: ./daily_standup.sh

set -euo pipefail

echo "=== Daily Standup for $(date +%Y-%m-%d) ==="
echo ""

echo "📅 Yesterday:"
k "recall what I worked on yesterday" | head -10
echo ""

echo "📋 Today's plan:"
k "recall any pending tasks or TODOs"
echo ""

echo "🚧 Blockers:"
k "recall any issues or blockers"
echo ""

echo "✅ Done!"
