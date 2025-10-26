#!/bin/bash
# © 2025 Casey Koons All rights reserved
# Scan codebase for improvement opportunities

set -e

# Colors for output
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "=========================================="
echo "KATRA CODE IMPROVEMENT SCAN"
echo "Generated: $(date '+%Y-%m-%d %H:%M:%S')"
echo "=========================================="
echo ""

# Get project root
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

# Source directories
SRC_CORE="src/core"
SRC_FOUNDATION="src/foundation"

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "1. FILE SIZE ANALYSIS"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Find files approaching 600-line limit
echo "Files approaching 600-line limit (500+ lines):"
find "$SRC_CORE" "$SRC_FOUNDATION" -name "*.c" -exec wc -l {} \; | \
    awk '$1 >= 500 {printf "  ⚠ %s: %d lines", $2, $1; if ($1 > 600) printf " (OVER LIMIT)"; printf "\n"}' | \
    sort -t: -k2 -rn

count=$(find "$SRC_CORE" "$SRC_FOUNDATION" -name "*.c" -exec wc -l {} \; | awk '$1 >= 500' | wc -l | tr -d ' ')
if [ "$count" -eq 0 ]; then
    echo "  ✓ No files approaching limit"
fi
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "2. DUPLICATE CODE DETECTION"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Find duplicate error handling patterns
echo "Common error handling patterns:"
grep -rn "if (!.*) {" "$SRC_CORE" "$SRC_FOUNDATION" --include="*.c" | \
    grep -E "(malloc|calloc|strdup|fopen)" | \
    wc -l | awk '{printf "  Memory allocation checks: %d occurrences\n", $1}'

grep -rn "goto cleanup" "$SRC_CORE" "$SRC_FOUNDATION" --include="*.c" | \
    wc -l | awk '{printf "  goto cleanup uses: %d occurrences\n", $1}'

grep -rn "katra_report_error" "$SRC_CORE" "$SRC_FOUNDATION" --include="*.c" | \
    wc -l | awk '{printf "  katra_report_error calls: %d occurrences\n", $1}'
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "3. REPEATED STRING PATTERNS"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Find most common error messages
echo "Most common error patterns (3+ occurrences):"
grep -roh '"Failed to [^"]*"' "$SRC_CORE" "$SRC_FOUNDATION" --include="*.c" | \
    sort | uniq -c | sort -rn | awk '$1 >= 3 {printf "  %2d × %s\n", $1, substr($0, index($0, $2))}'

count=$(grep -roh '"Failed to [^"]*"' "$SRC_CORE" "$SRC_FOUNDATION" --include="*.c" | \
    sort | uniq -c | sort -rn | awk '$1 >= 3' | wc -l | tr -d ' ')
if [ "$count" -eq 0 ]; then
    echo "  ✓ No repeated error patterns found (good variation)"
fi
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "4. FUNCTION COMPLEXITY"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Find longest functions (candidates for splitting)
echo "Longest functions (candidates for refactoring):"
python3 -c "
import sys
import re

def find_functions(filepath):
    with open(filepath, 'r') as f:
        lines = f.readlines()

    functions = []
    in_function = False
    func_name = None
    func_start = 0
    brace_count = 0

    for i, line in enumerate(lines, 1):
        # Skip comments and preprocessor
        stripped = line.strip()
        if stripped.startswith('//') or stripped.startswith('/*') or stripped.startswith('#'):
            continue

        # Look for function start
        if not in_function:
            # Match function definition pattern
            match = re.match(r'^(?:static\s+)?(?:inline\s+)?[\w\*]+\s+(\w+)\s*\([^)]*\)\s*\{', line)
            if match:
                func_name = match.group(1)
                func_start = i
                in_function = True
                brace_count = 1
        else:
            # Count braces
            brace_count += line.count('{') - line.count('}')
            if brace_count == 0:
                func_length = i - func_start + 1
                if func_length > 80:  # Only report functions > 80 lines
                    functions.append((func_name, filepath, func_start, func_length))
                in_function = False

    return functions

import os
all_funcs = []
for root, dirs, files in os.walk('$SRC_CORE'):
    for f in files:
        if f.endswith('.c'):
            all_funcs.extend(find_functions(os.path.join(root, f)))
for root, dirs, files in os.walk('$SRC_FOUNDATION'):
    for f in files:
        if f.endswith('.c'):
            all_funcs.extend(find_functions(os.path.join(root, f)))

all_funcs.sort(key=lambda x: x[3], reverse=True)
for name, path, line, length in all_funcs[:10]:
    print(f'  {length:3d} lines: {name}() at {path}:{line}')

if not all_funcs:
    print('  ✓ No functions over 80 lines')
" 2>/dev/null || echo "  (Python analysis unavailable)"
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "5. BOILERPLATE PATTERNS"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Find repeated initialization patterns
echo "Common initialization patterns:"
grep -rn "= NULL;" "$SRC_CORE" "$SRC_FOUNDATION" --include="*.c" | \
    wc -l | awk '{printf "  Pointer NULL initializations: %d\n", $1}'

grep -rn "= 0;" "$SRC_CORE" "$SRC_FOUNDATION" --include="*.c" | \
    wc -l | awk '{printf "  Zero initializations: %d\n", $1}'

grep -rn "calloc(1, sizeof" "$SRC_CORE" "$SRC_FOUNDATION" --include="*.c" | \
    wc -l | awk '{printf "  calloc() allocations: %d\n", $1}'

grep -rn "malloc.*sizeof" "$SRC_CORE" "$SRC_FOUNDATION" --include="*.c" | \
    wc -l | awk '{printf "  malloc() allocations: %d\n", $1}'
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "6. POTENTIAL MACRO OPPORTUNITIES"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Find repeated parameter validation patterns
echo "Parameter validation patterns (macro candidates):"
grep -rn "if (!.*) {" "$SRC_CORE" "$SRC_FOUNDATION" --include="*.c" | \
    grep -E "return E_INPUT_NULL" | wc -l | \
    awk '{printf "  NULL parameter checks: %d (consider CHECK_NULL macro)\n", $1}'

grep -rn "if (.*< 0) {" "$SRC_CORE" "$SRC_FOUNDATION" --include="*.c" | \
    grep -E "return E_INPUT_RANGE" | wc -l | \
    awk '{printf "  Range checks: %d (consider CHECK_RANGE macro)\n", $1}'
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "7. CROSS-FILE DUPLICATION"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Check for similar function names across files (potential candidates for shared utilities)
echo "Similar function names across files (potential for *_common.c):"
find "$SRC_CORE" "$SRC_FOUNDATION" -name "*.c" -exec grep -h "^[a-z_]*(" {} \; 2>/dev/null | \
    grep -v "^static" | sed 's/(.*$//' | sort | uniq -c | sort -rn | \
    awk '$1 >= 2 {printf "  %d files: %s\n", $1, $2}' | head -10

count=$(find "$SRC_CORE" "$SRC_FOUNDATION" -name "*.c" -exec grep -h "^[a-z_]*(" {} \; 2>/dev/null | \
    grep -v "^static" | sed 's/(.*$//' | sort | uniq -c | sort -rn | \
    awk '$1 >= 2' | wc -l | tr -d ' ')
if [ "$count" -eq 0 ]; then
    echo "  ✓ No obvious cross-file duplication"
fi
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "8. BUDGET STATUS"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Run the diet-aware line count
if [ -f "./scripts/dev/count_core.sh" ]; then
    ./scripts/dev/count_core.sh 2>&1 | grep -A 4 "Budget Status:"
else
    echo "  (count_core.sh not found)"
fi
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "9. RECENT CHANGES"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

echo "Recent commits (last 5):"
git log --oneline -5 2>/dev/null | sed 's/^/  /' || echo "  (git log unavailable)"
echo ""

echo "Files changed in last 3 commits:"
git diff --name-only HEAD~3..HEAD 2>/dev/null | grep -E "\.(c|h)$" | sed 's/^/  /' || echo "  (git diff unavailable)"
echo ""

echo "=========================================="
echo "SCAN COMPLETE"
echo "=========================================="
echo ""
echo "NEXT STEPS:"
echo "  1. Review findings above"
echo "  2. For AI-assisted analysis, run:"
echo "     /review-improvements"
echo "  3. Track high-priority items in improvement backlog"
echo ""
echo "This is automated detection only."
echo "Human judgment required for prioritization."
echo "=========================================="
