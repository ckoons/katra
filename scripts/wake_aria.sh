#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# wake_aria.sh - Launch Aria, the team chief of staff
#
# Usage: ./scripts/wake_aria.sh [working_directory]
#   working_directory: Optional. Defaults to the BST team notes, where the
#                      CI boards and backlog live.
#
# Aria converses with Casey and coordinates the team. She proposes dispatches
# and waits for Casey's nod before sending; when Casey manages the team
# directly (bin/katra-team-ui), she steps back until he hands the wheel back.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORK_DIR="${1:-$HOME/projects/github/BubbleSpacetimeTheory/notes}"

exec "$SCRIPT_DIR/wake_persona.sh" Aria "$WORK_DIR"
