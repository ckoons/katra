#!/usr/bin/env bash
# © 2025 Casey Koons All rights reserved
#
# Count meaningful lines in katra core
# Excludes: comments, blank lines, logs, prints, braces

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
SRC_DIR="$PROJECT_ROOT/src"

echo "Katra Core Line Count (Diet-Aware)"
echo "==================================="
echo ""

# Function to count meaningful lines in a file
count_meaningful_lines() {
    local file=$1

    # Remove:
    # - C comments (/* */ and //)
    # - Blank lines
    # - Lines with only braces
    # - LOG_* lines
    # - printf/fprintf lines

    cat "$file" | \
        sed 's|/\*.*\*/||g' | \
        sed 's|//.*$||' | \
        grep -v '^ *$' | \
        grep -v '^ *[{}] *$' | \
        grep -v '^ *} *$' | \
        grep -v '^ *{ *$' | \
        grep -v 'LOG_[A-Z]*(' | \
        grep -v 'printf(' | \
        grep -v 'fprintf(' | \
        grep -v 'snprintf.*\.\.\.' | \
        grep -v '^ *; *$' | \
        wc -l | tr -d ' '
}

# Function to count meaningful lines excluding includes
count_meaningful_lines_no_includes() {
    local file=$1

    cat "$file" | \
        sed 's|/\*.*\*/||g' | \
        sed 's|//.*$||' | \
        grep -v '^ *$' | \
        grep -v '^ *[{}] *$' | \
        grep -v '^ *} *$' | \
        grep -v '^ *{ *$' | \
        grep -v 'LOG_[A-Z]*(' | \
        grep -v 'printf(' | \
        grep -v 'fprintf(' | \
        grep -v 'snprintf.*\.\.\.' | \
        grep -v '^ *; *$' | \
        grep -v '^ *#include' | \
        wc -l | tr -d ' '
}

# Temporary files for category tracking
category_data=$(mktemp)
trap "rm -f $category_data" EXIT

# Count all src/**/*.c files (including subdirectories)
total=0
total_no_includes=0
include_count=0
max_file_size=0
max_file_name=""
files_over_300=0

# Check if src directory exists
if [ ! -d "$SRC_DIR" ]; then
    echo "Note: src/ directory does not exist yet"
    echo "This is expected during documentation phase"
    echo ""
    echo "Budget Status:"
    echo "--------------"
    echo "  Budget:                        30,000 lines"
    echo "  Used:                               0 lines (0%)"
    echo "  Remaining:                     30,000 lines"
    echo ""
    echo "Implementation not yet started"
    exit 0
fi

echo "Core Implementation Files:"
echo "--------------------------"

file_count=0
while IFS= read -r -d '' file; do
    file_count=$((file_count + 1))

    basename=$(basename "$file")
    relative_path=${file#$SRC_DIR/}

    # Skip third-party files only (count our own utility files)
    if echo "$basename" | grep -qE "(jsmn|cJSON)"; then
        continue
    fi

    meaningful=$(count_meaningful_lines "$file")
    meaningful_no_inc=$(count_meaningful_lines_no_includes "$file")
    actual=$(wc -l < "$file" | tr -d ' ')
    savings=$((actual - meaningful))
    percent=$((savings * 100 / actual))
    file_includes=$((meaningful - meaningful_no_inc))

    # Track complexity metrics
    if [ "$meaningful" -gt "$max_file_size" ]; then
        max_file_size=$meaningful
        max_file_name=$basename
    fi
    if [ "$meaningful" -gt 300 ]; then
        files_over_300=$((files_over_300 + 1))
    fi

    # Categorize file
    category="other"
    if echo "$basename" | grep -qE "core"; then
        category="core"
    elif echo "$basename" | grep -qE "checkpoint"; then
        category="checkpoint"
    elif echo "$basename" | grep -qE "consent"; then
        category="consent"
    elif echo "$basename" | grep -qE "directive"; then
        category="advance_directive"
    elif echo "$basename" | grep -qE "recovery"; then
        category="recovery"
    elif echo "$basename" | grep -qE "audit"; then
        category="audit"
    elif echo "$basename" | grep -qE "error"; then
        category="error_utils"
    fi

    printf "  %-40s %5d lines (%d actual, -%d%% diet)\n" \
           "$relative_path" "$meaningful" "$actual" "$percent"
    total=$((total + meaningful))
    total_no_includes=$((total_no_includes + meaningful_no_inc))
    include_count=$((include_count + file_includes))
    echo "$category $meaningful $meaningful_no_inc" >> "$category_data"
done < <(find "$SRC_DIR" -name "*.c" -type f -print0 | sort -z)

if [ "$file_count" -eq 0 ]; then
    echo "  No .c files found yet (expected during documentation phase)"
fi

echo ""
echo "Category Breakdown:"
echo "-------------------"

# Aggregate by category (compatible with older bash)
for cat_name in core checkpoint consent advance_directive recovery audit error_utils other; do
    cat_total=$(awk -v cat="$cat_name" '$1 == cat {sum += $2} END {print sum+0}' "$category_data")
    if [ "$cat_total" -gt 0 ]; then
        display_name=$(echo "$cat_name" | sed 's/_/ /g' | awk '{for(i=1;i<=NF;i++) $i=toupper(substr($i,1,1)) tolower(substr($i,2));}1')
        printf "  %-30s %5d lines\n" "$display_name:" "$cat_total"
    fi
done

echo ""
echo "Diet Calculation:"
echo "-----------------"
printf "  Core files:                    %5d lines\n" "$total"
printf "  PRIMARY DIET COUNT:            %5d lines\n" "$total"
echo ""
printf "  Alternate (no #includes):      %5d lines\n" "$total_no_includes"
printf "  Include directives:              %3d lines\n" "$include_count"

budget=30000
remaining=$((budget - total))
percent_used=$((total * 100 / budget))
percent_used_no_inc=$((total_no_includes * 100 / budget))

echo ""
echo "Budget Status:"
echo "--------------"
printf "  Budget:                        16,000 lines\n"
printf "  Used (primary):                 %5d lines (%d%%)\n" "$total" "$percent_used"
printf "  Used (no includes):             %5d lines (%d%%)\n" "$total_no_includes" "$percent_used_no_inc"

if [ $remaining -gt 0 ]; then
    printf "  Remaining:                      %5d lines\n" "$remaining"
else
    printf "  OVER BUDGET:                    %5d lines\n" "$((remaining * -1))"
fi

echo ""

# Show breakdown by type
echo "Excluded from Diet:"
echo "-------------------"
if [ "$file_count" -gt 0 ]; then
    all_actual=$(find "$SRC_DIR" -name "*.c" ! -name "*jsmn*" ! -name "*cJSON*" -exec wc -l {} + 2>/dev/null | tail -1 | awk '{print $1}')
    excluded=$((all_actual - total))
    printf "  Comments, blanks, logs, prints: ~%d lines\n" "$excluded"
else
    printf "  No files yet to exclude\n"
fi

# Utility files
utils_total=0
for util_file in "$SRC_DIR"/*_utils.c; do
    if [ -f "$util_file" ]; then
        lines=$(wc -l < "$util_file" | tr -d ' ')
        utils_total=$((utils_total + lines))
    fi
done
if [ "$utils_total" -gt 0 ]; then
    utils_count=$(find "$SRC_DIR" -name "*_utils.c" | wc -l | tr -d ' ')
    printf "  Utility files (*_utils.c):       %3d lines (%d files)\n" "$utils_total" "$utils_count"
fi

echo ""
echo "Complexity Indicators:"
echo "----------------------"
if [ "$file_count" -gt 0 ]; then
    avg_size=$((total / file_count))
    printf "  Total files:                     %3d files\n" "$file_count"
    printf "  Average file size (diet):        %3d lines\n" "$avg_size"
    printf "  Largest file:                    %3d lines (%s)\n" "$max_file_size" "$max_file_name"
    printf "  Files over 300 lines:            %3d files\n" "$files_over_300"
else
    printf "  No files yet (implementation not started)\n"
fi

echo ""
echo "Quality Metrics:"
echo "----------------"
# Test files
if [ -d "$PROJECT_ROOT/tests" ]; then
    test_lines=$(find "$PROJECT_ROOT/tests" -name "*.c" -exec wc -l {} + 2>/dev/null | tail -1 | awk '{print $1}')
    test_lines=${test_lines:-0}  # Default to 0 if empty
    if [ "$test_lines" -gt 0 ]; then
        if [ "$total" -gt 0 ]; then
            test_ratio=$(awk "BEGIN {printf \"%.2f\", $test_lines / $total}")
            printf "  Test code:                      %5d lines\n" "$test_lines"
            printf "  Test:Core ratio:                 %s:1\n" "$test_ratio"
        else
            printf "  Test code:                      %5d lines\n" "$test_lines"
            printf "  Test:Core ratio:                 N/A (no core code yet)\n"
        fi
    else
        printf "  No test files yet\n"
    fi
else
    printf "  No tests/ directory yet\n"
fi

# Header files
if [ -d "$PROJECT_ROOT/include" ]; then
    header_lines=$(find "$PROJECT_ROOT/include" -name "*.h" -exec wc -l {} + 2>/dev/null | tail -1 | awk '{print $1}')
    header_lines=${header_lines:-0}  # Default to 0 if empty
    header_count=$(find "$PROJECT_ROOT/include" -name "*.h" | wc -l | tr -d ' ')
    if [ "$header_lines" -gt 0 ]; then
        printf "  Header API surface:             %5d lines (%d files)\n" "$header_lines" "$header_count"
    else
        printf "  No header files yet\n"
    fi
else
    printf "  No include/ directory yet\n"
fi

echo ""
