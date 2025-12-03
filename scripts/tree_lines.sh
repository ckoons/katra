#!/bin/bash
# © 2025 Casey Koons All rights reserved
# tree_lines.sh - Display directory tree with line counts
#
# Usage: ./scripts/tree_lines.sh [directory] [options]
#
# Options:
#   -e, --ext EXT    Filter by file extension (e.g., -e c -e h)
#   -t, --total      Show only grand total
#   -h, --help       Show this help message

set -e

# Colors (disable if not a terminal)
if [ -t 1 ]; then
    DIR_COLOR="\033[1;34m"
    FILE_COLOR="\033[0m"
    COUNT_COLOR="\033[0;33m"
    TOTAL_COLOR="\033[1;32m"
    SUBTOTAL_COLOR="\033[0;36m"
    RESET="\033[0m"
else
    DIR_COLOR=""
    FILE_COLOR=""
    COUNT_COLOR=""
    TOTAL_COLOR=""
    SUBTOTAL_COLOR=""
    RESET=""
fi

# Defaults
TARGET_DIR="."
EXTENSIONS=""
TOTAL_ONLY=false

# Parse arguments
while [ $# -gt 0 ]; do
    case $1 in
        -e|--ext)
            if [ -z "$EXTENSIONS" ]; then
                EXTENSIONS="$2"
            else
                EXTENSIONS="$EXTENSIONS|$2"
            fi
            shift 2
            ;;
        -t|--total)
            TOTAL_ONLY=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [directory] [options]"
            echo ""
            echo "Display directory tree with line counts for each file."
            echo ""
            echo "Options:"
            echo "  -e, --ext EXT    Filter by file extension (can be used multiple times)"
            echo "                   Example: -e c -e h"
            echo "  -t, --total      Show only grand total"
            echo "  -h, --help       Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0 src/                    # Show all files in src/"
            echo "  $0 src/ -e c               # Show only .c files"
            echo "  $0 src/ -e c -e h          # Show .c and .h files"
            exit 0
            ;;
        -*)
            echo "Unknown option: $1" >&2
            exit 1
            ;;
        *)
            TARGET_DIR="$1"
            shift
            ;;
    esac
done

# Validate directory
if [ ! -d "$TARGET_DIR" ]; then
    echo "Error: '$TARGET_DIR' is not a directory" >&2
    exit 1
fi

# Track totals
GRAND_TOTAL=0
GRAND_FILES=0

# Temporary file for directory summaries
DIR_SUMMARY_FILE=$(mktemp)
trap "rm -f $DIR_SUMMARY_FILE" EXIT

# Function to check if file matches extension filter
matches_extension() {
    local file="$1"
    if [ -z "$EXTENSIONS" ]; then
        return 0  # No filter, match all
    fi
    local ext="${file##*.}"
    echo "$EXTENSIONS" | grep -qE "(^|\\|)$ext($|\\|)"
}

# Function to print tree structure recursively
# Returns: total lines for this directory (including subdirs)
print_tree() {
    local dir="$1"
    local prefix="$2"
    local depth="$3"
    local dir_total=0
    local dir_files=0

    # Get sorted list of items
    local items=$(ls -1 "$dir" 2>/dev/null | sort)
    local total_items=$(echo "$items" | grep -c . || echo 0)
    local current=0

    for item in $items; do
        local path="$dir/$item"
        current=$((current + 1))

        # Determine if last item
        local connector="├──"
        local next_prefix="${prefix}│   "
        if [ $current -eq $total_items ]; then
            connector="└──"
            next_prefix="${prefix}    "
        fi

        if [ -d "$path" ]; then
            # Directory - print name first
            printf "${prefix}${connector} ${DIR_COLOR}%s/${RESET}" "$item"

            # Recurse and get subtotal
            local subdir_total=$(print_tree "$path" "$next_prefix" $((depth + 1)))
            dir_total=$((dir_total + subdir_total))

        elif [ -f "$path" ]; then
            # File - check extension filter
            if matches_extension "$item"; then
                local lines=$(wc -l < "$path" 2>/dev/null | tr -d ' ')
                printf "${prefix}${connector} ${FILE_COLOR}%s${RESET}  ${COUNT_COLOR}%s${RESET}\n" "$item" "$lines"
                dir_total=$((dir_total + lines))
                dir_files=$((dir_files + 1))
                GRAND_TOTAL=$((GRAND_TOTAL + lines))
                GRAND_FILES=$((GRAND_FILES + 1))
            fi
        fi
    done

    # Store directory summary if it has files (directly or in subdirs)
    if [ $dir_total -gt 0 ]; then
        local rel_path="${dir#$TARGET_DIR}"
        [ -z "$rel_path" ] && rel_path="."
        printf "%d\t%s\n" "$dir_total" "$rel_path" >> "$DIR_SUMMARY_FILE"
    fi

    # Return total for parent to use
    echo "$dir_total"
}

# Modified print_tree that shows directory totals inline
print_tree_with_totals() {
    local dir="$1"
    local prefix="$2"
    local dir_total=0
    local dir_files=0

    # Get sorted list of items - directories first, then files
    local dirs=$(find "$dir" -maxdepth 1 -mindepth 1 -type d 2>/dev/null | sort)
    local files=""

    if [ -z "$EXTENSIONS" ]; then
        files=$(find "$dir" -maxdepth 1 -mindepth 1 -type f 2>/dev/null | sort)
    else
        for ext in $(echo "$EXTENSIONS" | tr '|' ' '); do
            files="$files $(find "$dir" -maxdepth 1 -mindepth 1 -type f -name "*.$ext" 2>/dev/null)"
        done
        files=$(echo "$files" | tr ' ' '\n' | grep -v '^$' | sort)
    fi

    local all_items=$(echo -e "${dirs}\n${files}" | grep -v '^$')
    local total_items=$(echo "$all_items" | grep -c . || echo 0)
    local current=0

    # Process directories
    for subdir in $dirs; do
        [ -z "$subdir" ] && continue
        current=$((current + 1))
        local item=$(basename "$subdir")

        local connector="├──"
        local next_prefix="${prefix}│   "
        if [ $current -eq $total_items ]; then
            connector="└──"
            next_prefix="${prefix}    "
        fi

        # Calculate subtotal first
        local subdir_total=$(calculate_dir_total "$subdir")

        if [ $subdir_total -gt 0 ]; then
            printf "${prefix}${connector} ${DIR_COLOR}%s/${RESET}  ${SUBTOTAL_COLOR}[%d lines]${RESET}\n" "$item" "$subdir_total"
            print_tree_with_totals "$subdir" "$next_prefix"
            dir_total=$((dir_total + subdir_total))
        else
            printf "${prefix}${connector} ${DIR_COLOR}%s/${RESET}\n" "$item"
        fi
    done

    # Process files
    for filepath in $files; do
        [ -z "$filepath" ] && continue
        [ ! -f "$filepath" ] && continue
        current=$((current + 1))
        local item=$(basename "$filepath")

        local connector="├──"
        if [ $current -eq $total_items ]; then
            connector="└──"
        fi

        local lines=$(wc -l < "$filepath" 2>/dev/null | tr -d ' ')
        printf "${prefix}${connector} ${FILE_COLOR}%s${RESET}  ${COUNT_COLOR}%s${RESET}\n" "$item" "$lines"
        dir_total=$((dir_total + lines))
        GRAND_TOTAL=$((GRAND_TOTAL + lines))
        GRAND_FILES=$((GRAND_FILES + 1))
    done
}

# Calculate total lines for a directory (recursive)
calculate_dir_total() {
    local dir="$1"
    local total=0

    if [ -z "$EXTENSIONS" ]; then
        total=$(find "$dir" -type f -exec cat {} + 2>/dev/null | wc -l | tr -d ' ')
    else
        for ext in $(echo "$EXTENSIONS" | tr '|' ' '); do
            local ext_total=$(find "$dir" -type f -name "*.$ext" -exec cat {} + 2>/dev/null | wc -l | tr -d ' ')
            total=$((total + ext_total))
        done
    fi

    echo "$total"
}

# Function for total-only mode
count_total() {
    local total=0
    local files=0

    if [ -n "$EXTENSIONS" ]; then
        for ext in $(echo "$EXTENSIONS" | tr '|' ' '); do
            while IFS= read -r file; do
                if [ -n "$file" ]; then
                    local lines=$(wc -l < "$file" 2>/dev/null | tr -d ' ')
                    total=$((total + lines))
                    files=$((files + 1))
                fi
            done < <(find "$TARGET_DIR" -type f -name "*.$ext" 2>/dev/null)
        done
    else
        while IFS= read -r file; do
            if [ -n "$file" ]; then
                local lines=$(wc -l < "$file" 2>/dev/null | tr -d ' ')
                total=$((total + lines))
                files=$((files + 1))
            fi
        done < <(find "$TARGET_DIR" -type f 2>/dev/null)
    fi

    printf "${TOTAL_COLOR}Total: %d lines in %d files${RESET}\n" "$total" "$files"
}

# Main execution
if [ "$TOTAL_ONLY" = true ]; then
    count_total
else
    # Calculate root total first
    ROOT_TOTAL=$(calculate_dir_total "$TARGET_DIR")

    # Print root directory with total
    printf "${DIR_COLOR}%s${RESET}  ${SUBTOTAL_COLOR}[%d lines total]${RESET}\n" "$(cd "$TARGET_DIR" && pwd)" "$ROOT_TOTAL"

    print_tree_with_totals "$TARGET_DIR" ""

    echo ""
    printf "${TOTAL_COLOR}════════════════════════════════════════${RESET}\n"
    printf "${TOTAL_COLOR}Total: %d lines in %d files${RESET}\n" "$GRAND_TOTAL" "$GRAND_FILES"

    # Print directory summary
    echo ""
    printf "${TOTAL_COLOR}Directory Summary:${RESET}\n"
    printf "${TOTAL_COLOR}────────────────────────────────────────${RESET}\n"

    # Get all directories with their totals, sorted by total descending
    if [ -z "$EXTENSIONS" ]; then
        find "$TARGET_DIR" -type d 2>/dev/null | while read -r subdir; do
            rel_path="${subdir#$TARGET_DIR}"
            [ -z "$rel_path" ] && rel_path="."
            dir_total=$(find "$subdir" -maxdepth 1 -type f -exec cat {} + 2>/dev/null | wc -l | tr -d ' ')
            dir_files=$(find "$subdir" -maxdepth 1 -type f 2>/dev/null | wc -l | tr -d ' ')
            if [ "$dir_files" -gt 0 ]; then
                printf "%d\t%d\t%s\n" "$dir_total" "$dir_files" "$rel_path"
            fi
        done | sort -rn | while IFS=$'\t' read -r lines files path; do
            printf "  ${DIR_COLOR}%-40s${RESET} ${COUNT_COLOR}%6d${RESET} lines  ${FILE_COLOR}%3d${RESET} files\n" "$path" "$lines" "$files"
        done
    else
        find "$TARGET_DIR" -type d 2>/dev/null | while read -r subdir; do
            rel_path="${subdir#$TARGET_DIR}"
            [ -z "$rel_path" ] && rel_path="."
            dir_total=0
            dir_files=0
            for ext in $(echo "$EXTENSIONS" | tr '|' ' '); do
                ext_total=$(find "$subdir" -maxdepth 1 -type f -name "*.$ext" -exec cat {} + 2>/dev/null | wc -l | tr -d ' ')
                ext_files=$(find "$subdir" -maxdepth 1 -type f -name "*.$ext" 2>/dev/null | wc -l | tr -d ' ')
                dir_total=$((dir_total + ext_total))
                dir_files=$((dir_files + ext_files))
            done
            if [ "$dir_files" -gt 0 ]; then
                printf "%d\t%d\t%s\n" "$dir_total" "$dir_files" "$rel_path"
            fi
        done | sort -rn | while IFS=$'\t' read -r lines files path; do
            printf "  ${DIR_COLOR}%-40s${RESET} ${COUNT_COLOR}%6d${RESET} lines  ${FILE_COLOR}%3d${RESET} files\n" "$path" "$lines" "$files"
        done
    fi
fi
