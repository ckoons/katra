Katra Project Setup Guide - Argo Code Discipline

  Overview

  This guide establishes code discipline and project structure for Katra, following proven patterns
  from the Argo project. Use this as the foundation for all development work.

  ---
  Directory Structure

  katra/
  ├── src/              # Source code (.c files)
  │   ├── katra_*.c     # Core modules
  │   └── (organized by layer)
  ├── include/          # Header files (.h files)
  │   ├── katra_*.h     # Public interfaces
  │   └── katra_limits.h # ALL numeric constants
  ├── build/            # Intermediate build artifacts (.o, .a)
  │   └── (git ignored, safe to delete)
  ├── bin/              # Final executable deliverables
  │   ├── katra         # Main executable
  │   ├── utils/        # Developer/operator tools
  │   └── tests/        # Test executables
  ├── scripts/          # Shell scripts + utilities
  │   ├── *.sh          # Build/test/utility scripts
  │   └── count_core.sh # Line counter
  ├── tests/            # Test source files
  │   ├── test_*.c      # Unit/integration tests
  │   └── run_tests.sh  # Test runner
  ├── docs/             # Documentation (create only when needed)
  ├── .env.katra        # Build-time configuration (committed)
  ├── .env.katra.local  # Local overrides (NOT committed, .gitignore)
  ├── Makefile          # Build system
  └── README.md         # Project overview

  Key Principles:
  - src/ - Source only (tracked in git)
  - build/ - Intermediate artifacts (git ignored, deletable)
  - bin/ - Final executables (git ignored, generated)
  - make clean removes build/ and bin/ completely

  ---
  Code Discipline - Iron Rules

  1. Copyright & Files

  /* © 2025 Casey Koons All rights reserved */
  - Add to ALL source and header files
  - Never create files without explicit request
  - Never create documentation files unless asked
  - Prefer editing existing files over creating new ones

  2. Memory Safety Checklist

  Every function that allocates resources:
  - Check ALL return values (malloc, calloc, strdup, fopen, etc.)
  - Free everything you allocate (no leaks)
  - Use goto cleanup pattern (mandatory for resource cleanup)
  - Initialize ALL variables at declaration
  - NULL-check all pointer parameters at function entry
  - Never return uninitialized pointers

  goto cleanup pattern (mandatory):
  int katra_operation(void) {
      int result = 0;
      char* buffer = NULL;
      FILE* file = NULL;

      buffer = malloc(SIZE);
      if (!buffer) {
          result = E_SYSTEM_MEMORY;
          goto cleanup;
      }

      file = fopen(path, "r");
      if (!file) {
          result = E_FILE_OPEN;
          goto cleanup;
      }

      // ... operation logic ...

  cleanup:
      free(buffer);           // Safe even if NULL
      if (file) fclose(file);
      return result;
  }

  3. String Safety - Banned Functions

  NEVER use these unsafe functions:
  - ❌ gets - Use fgets instead
  - ❌ strcpy - Use strncpy + explicit null termination
  - ❌ sprintf - Use snprintf always
  - ❌ strcat - Use strncat with size limits

  Always use safe alternatives:
  /* WRONG - unsafe */
  strcpy(buffer, source);
  sprintf(buffer, "%s", str);

  /* CORRECT - safe with size limits */
  strncpy(buffer, source, sizeof(buffer) - 1);
  buffer[sizeof(buffer) - 1] = '\0';  /* Explicit null termination */

  snprintf(buffer, sizeof(buffer), "%s", str);

  /* BEST - offset tracking for multiple operations */
  size_t offset = 0;
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "Part 1: %s\n", part1);
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "Part 2: %s\n", part2);

  4. Error Reporting

  Centralized error reporting (single breakpoint for debugging):
  /* In katra_error.h */
  int katra_report_error(int error_code, const char* function, const char* details);

  /* Usage - ONLY way to report errors */
  katra_report_error(E_FILE_NOT_FOUND, "katra_load_config", filename);

  /* NEVER use fprintf(stderr, ...) for errors - use katra_report_error() */

  Acceptable stderr usage:
  - Daemon startup diagnostics (before logging initialized)
  - Usage messages and help text
  - Progress reporting (non-error status)
  - Never for error reporting - use katra_report_error() exclusively

  5. Constants - No Magic Numbers

  ALL numeric constants in headers:
  /* katra_limits.h - ALL numeric constants live here */

  /* Buffer sizes */
  #define KATRA_BUFFER_TINY     32
  #define KATRA_BUFFER_SMALL    64
  #define KATRA_BUFFER_MEDIUM   256
  #define KATRA_BUFFER_STANDARD 4096
  #define KATRA_BUFFER_LARGE    16384

  /* Filesystem */
  #define KATRA_PATH_MAX        512
  #define KATRA_DIR_PERMISSIONS 0755
  #define KATRA_FILE_PERMISSIONS 0644

  /* Timeouts, ports, limits, etc. */
  #define KATRA_HTTP_TIMEOUT_SECONDS 30
  #define KATRA_MAX_CONNECTIONS      100

  Rule: NEVER use numeric literals in .c files
  - Exception: Array indices, loop counters (0, 1, -1)
  - Exception: Return codes (0, 1) in main()
  - Everything else must be a named constant in a header

  Verify compliance:
  grep -n '[^a-zA-Z_][0-9]{2,}' src/katra_module.c
  # Should find NOTHING except line numbers

  6. No String Literals in .c Files

  ALL strings in headers (except format strings):
  /* katra_messages.h */
  #define MSG_STARTUP "Katra daemon starting..."
  #define MSG_SHUTDOWN "Katra daemon shutting down..."
  #define ERR_MEMORY_ALLOC "Failed to allocate memory"

  /* Usage in .c file */
  LOG_INFO(MSG_STARTUP);
  katra_report_error(E_SYSTEM_MEMORY, __func__, ERR_MEMORY_ALLOC);

  Acceptable string literals in .c files:
  - Format strings: "%s", "%d", "Error: %s in %s"
  - JSON field names in jq-style operations
  - Everything else must be a constant in a header

  ---
  Code Organization

  File Size Limits

  - Max 600 lines per .c file (3% tolerance = 618 lines acceptable)
  - Max 100 lines per function (refactor if longer)
  - One clear purpose per module

  When approaching 600 lines:
  1. Split into logical sub-modules
  2. Example: katra_http.c → katra_http.c + katra_http_client.c + katra_http_server.c

  Module Layering

  Organize code in layers (bottom to top):
  1. Foundation   - Error, logging, utilities
  2. Core         - Domain-specific abstractions
  3. Services     - Higher-level functionality
  4. Application  - Main executable logic

  Dependency rule: Lower layers never depend on higher layers.

  Header Organization

  /* katra_module.h */
  /* © 2025 Casey Koons All rights reserved */

  #ifndef KATRA_MODULE_H
  #define KATRA_MODULE_H

  #include <stddef.h>  /* System includes first */
  #include <stdint.h>

  #include "katra_error.h"  /* Project includes second */

  /* Constants */
  #define KATRA_MODULE_BUFFER_SIZE 1024

  /* Type definitions */
  typedef struct katra_context {
      int ref_count;
      char* buffer;
  } katra_context_t;

  /* Public function declarations */
  int katra_module_init(katra_context_t* ctx);
  void katra_module_cleanup(katra_context_t* ctx);

  #endif /* KATRA_MODULE_H */

  Source File Organization

  /* katra_module.c */
  /* © 2025 Casey Koons All rights reserved */

  /* System includes */
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>

  /* Project includes */
  #include "katra_module.h"
  #include "katra_error.h"
  #include "katra_limits.h"

  /* Static function declarations */
  static int helper_function(const char* input);

  /* Public functions */
  int katra_module_init(katra_context_t* ctx) {
      if (!ctx) {
          katra_report_error(E_INVALID_PARAMS, __func__, "ctx is NULL");
          return E_INVALID_PARAMS;
      }

      /* implementation */
      return KATRA_SUCCESS;
  }

  /* Static functions */
  static int helper_function(const char* input) {
      /* implementation */
  }

  ---
  Naming Conventions

  /* Types - lowercase with _t suffix */
  typedef struct katra_context katra_context_t;
  typedef enum katra_state katra_state_t;

  /* Functions - prefix with module name */
  int katra_module_init(void);
  static int helper_function(void);  /* static = no prefix needed */

  /* Constants - UPPERCASE with KATRA_ prefix */
  #define KATRA_BUFFER_SIZE 4096
  #define KATRA_SUCCESS 0

  /* Enums - UPPERCASE */
  typedef enum {
      KATRA_STATE_INIT,
      KATRA_STATE_RUNNING,
      KATRA_STATE_ERROR
  } katra_state_t;

  /* Variables - lowercase with underscores */
  int result_code = 0;
  char* buffer_ptr = NULL;

  Wrong (don't use CamelCase in C):
  typedef struct KatraContext { ... }  // NO
  void KatraModuleInit(...)            // NO

  ---
  Build System

  Makefile Structure

  # Compiler settings
  CC = gcc
  CFLAGS = -Wall -Werror -Wextra -std=c11 -I./include
  LDFLAGS = -lcurl -ljson-c

  # Directories
  SRC_DIR = src
  BUILD_DIR = build
  BIN_DIR = bin
  TEST_DIR = tests

  # Source files
  SOURCES = $(wildcard $(SRC_DIR)/katra_*.c)
  OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

  # Targets
  all: $(BIN_DIR)/katra

  $(BIN_DIR)/katra: $(OBJECTS) | $(BIN_DIR)
  	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

  $(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
  	$(CC) $(CFLAGS) -c $< -o $@

  $(BIN_DIR) $(BUILD_DIR):
  	mkdir -p $@

  clean:
  	rm -rf $(BUILD_DIR) $(BIN_DIR)

  test: $(BIN_DIR)/katra
  	@./scripts/run_tests.sh

  .PHONY: all clean test

  Compilation Requirements

  Must compile with zero warnings:
  gcc -Wall -Werror -Wextra -std=c11 -c file.c

  - -Wall - All warnings
  - -Werror - Treat warnings as errors
  - -Wextra - Extra warnings
  - -std=c11 - C11 standard

  Before every commit:
  make clean && make && make test

  ---
  Testing Infrastructure

  Test Framework Pattern

  /* tests/test_framework.h */
  void assert_equals(int expected, int actual, const char* msg);
  void assert_not_null(void* ptr, const char* msg);
  void assert_success(int result, const char* msg);
  void assert_failure(int result, const char* msg);

  Test Structure

  /* tests/test_module.c */
  #include "test_framework.h"
  #include "katra_module.h"

  void test_module_init() {
      katra_context_t ctx = {0};
      int result = katra_module_init(&ctx);
      assert_success(result, "Module init should succeed");
  }

  void test_module_null_param() {
      int result = katra_module_init(NULL);
      assert_failure(result, "Should reject NULL parameter");
  }

  int main() {
      printf("Running module tests...\n");
      test_module_init();
      test_module_null_param();
      printf("All tests passed!\n");
      return 0;
  }

  Test Runner Script

  #!/bin/bash
  # scripts/run_tests.sh

  TESTS_PASSED=0
  TESTS_FAILED=0

  for test in bin/tests/test_*; do
      if [[ -x "$test" ]]; then
          echo "Running $(basename $test)..."
          if "$test"; then
              ((TESTS_PASSED++))
          else
              ((TESTS_FAILED++))
              echo "FAILED: $(basename $test)"
          fi
      fi
  done

  echo "Tests passed: $TESTS_PASSED"
  echo "Tests failed: $TESTS_FAILED"
  [[ $TESTS_FAILED -eq 0 ]] && exit 0 || exit 1

  ---
  Line Count Budgeting

  Core Budget

  Initial target: 10,000 meaningful lines
  - Excludes blank lines, comments, braces
  - Run: ./scripts/count_core.sh

  Line Counter Script

  #!/bin/bash
  # scripts/count_core.sh - Count meaningful lines

  find src -name "*.c" -exec cat {} \; | \
    sed '/^[[:space:]]*$/d' | \
    sed '/^[[:space:]]*\/\//d' | \
    sed '/^[[:space:]]*\/\*/d' | \
    sed '/^[[:space:]]*\*\//d' | \
    sed '/^[[:space:]]*\*[^\/]/d' | \
    sed '/^[[:space:]]*{[[:space:]]*$/d' | \
    sed '/^[[:space:]]*}[[:space:]]*$/d' | \
    wc -l

  Monitor growth:
  - Check regularly: ./scripts/count_core.sh
  - Stay under budget
  - Refactor when approaching limits

  ---
  Common Patterns

  Error Handling (goto cleanup)

  int katra_complex_operation(const char* path) {
      int result = KATRA_SUCCESS;
      FILE* file = NULL;
      char* buffer = NULL;
      size_t buffer_size = KATRA_BUFFER_STANDARD;

      if (!path) {
          result = E_INVALID_PARAMS;
          goto cleanup;
      }

      buffer = malloc(buffer_size);
      if (!buffer) {
          katra_report_error(E_SYSTEM_MEMORY, __func__, "buffer allocation");
          result = E_SYSTEM_MEMORY;
          goto cleanup;
      }

      file = fopen(path, "r");
      if (!file) {
          katra_report_error(E_FILE_OPEN, __func__, path);
          result = E_FILE_OPEN;
          goto cleanup;
      }

      /* ... operation logic ... */

  cleanup:
      if (file) fclose(file);
      free(buffer);  /* Safe even if NULL */
      return result;
  }

  Null Check Macro

  /* katra_common.h */
  #define KATRA_CHECK_NULL(ptr) \
      do { \
          if (!(ptr)) { \
              katra_report_error(E_INVALID_PARAMS, __func__, #ptr " is NULL"); \
              return E_INVALID_PARAMS; \
          } \
      } while(0)

  /* Usage */
  int katra_operation(katra_context_t* ctx, const char* name) {
      KATRA_CHECK_NULL(ctx);
      KATRA_CHECK_NULL(name);
      /* ... */
  }

  Buffer Allocation Helper

  /* Ensure buffer has capacity, reallocate if needed */
  int ensure_buffer_capacity(char** buffer, size_t* current_capacity, 
                            size_t required_capacity) {
      if (*current_capacity >= required_capacity) {
          return KATRA_SUCCESS;
      }

      size_t new_capacity = required_capacity * 2;  /* Grow by 2x */
      char* new_buffer = realloc(*buffer, new_capacity);
      if (!new_buffer) {
          katra_report_error(E_SYSTEM_MEMORY, __func__, "buffer resize");
          return E_SYSTEM_MEMORY;
      }

      *buffer = new_buffer;
      *current_capacity = new_capacity;
      return KATRA_SUCCESS;
  }

  ---
  Pre-Commit Checklist

  Before committing ANY code:
  - Copyright header on all new/modified files
  - Zero magic numbers in .c files (except 0, 1, -1)
  - Zero string literals in .c files (except format strings)
  - No unsafe string functions (strcpy, sprintf, strcat, gets)
  - All resource allocations use goto cleanup pattern
  - Compiles with gcc -Wall -Werror -Wextra
  - All tests pass: make test
  - Line count under budget: ./scripts/count_core.sh
  - No files over 600 lines (3% tolerance)
  - No functions over 100 lines

  Verify command:
  make clean && make && make test && ./scripts/count_core.sh

  ---
  Philosophy

  "What you don't build, you don't debug."

  Before writing code:
  1. Search existing code for similar patterns
  2. Check utility headers for helpers
  3. Reuse or refactor existing code
  4. Only create new code when truly necessary

  Simple beats clever

  - Obvious code over clever code
  - Working code over perfect code
  - Follow patterns that work
  - Consistency over innovation

  Code you'd debug at 3am

  - Clear variable names
  - Obvious logic flow
  - Comprehensive error messages
  - Single breakpoint for errors (katra_report_error)

  ---
  Essential Scripts

  count_core.sh

  #!/bin/bash
  # Count meaningful lines (exclude blanks, comments, braces)
  find src -name "*.c" -exec cat {} \; | \
    sed '/^[[:space:]]*$/d' | \
    sed '/^[[:space:]]*\/\//d' | \
    sed '/^[[:space:]]*\/\*/d' | \
    sed '/^[[:space:]]*\*\//d' | \
    sed '/^[[:space:]]*\*[^\/]/d' | \
    sed '/^[[:space:]]*{[[:space:]]*$/d' | \
    sed '/^[[:space:]]*}[[:space:]]*$/d' | \
    wc -l

  run_tests.sh

  #!/bin/bash
  # Run all test executables
  set -e

  PASSED=0
  FAILED=0

  for test in bin/tests/test_*; do
      [[ -x "$test" ]] || continue
      echo "Running $(basename $test)..."
      if "$test"; then
          ((PASSED++))
      else
          ((FAILED++))
      fi
  done

  echo "Passed: $PASSED, Failed: $FAILED"
  exit $FAILED

  ---
  Quick Reference

  Module Creation:
  1. Create include/katra_module.h with header guards
  2. Create src/katra_module.c with copyright
  3. Add to Makefile SOURCES
  4. Create tests/test_module.c
  5. Verify: make clean && make && make test

  Daily Workflow:
  # Before starting work
  make clean && make

  # During development
  make                    # Incremental build
  ./bin/katra            # Test executable
  make test              # Run tests

  # Before commit
  make clean && make && make test
  ./scripts/count_core.sh
  git add ...
  git commit -m "..."

  Debugging:
  - All errors route through katra_report_error() - set ONE breakpoint
  - Use -g flag for debug builds: CFLAGS += -g
  - Run with valgrind: valgrind --leak-check=full ./bin/katra

  ---
  This discipline has been battle-tested in the Argo project. Follow these patterns exactly, and your
  code will be maintainable, debuggable, and robust. When in doubt, look at Argo code for examples.
 ADDENDUM: Actual Argo Scripts to Copy

  1. Line Counter Script (scripts/dev/count_core.sh)

  From Argo - Copy and adapt this exact script for Katra:

  #!/usr/bin/env bash
  # © 2025 Casey Koons All rights reserved
  #
  # Count meaningful lines in katra core
  # Excludes: comments, blank lines, logs, prints, braces

  set -e

  SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
  PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
  SRC_DIR="$PROJECT_ROOT/src"

  echo "Katra Core Line Count"
  echo "====================="
  echo ""

  count_meaningful_lines() {
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
          wc -l | tr -d ' '
  }

  total=0
  max_file_size=0
  max_file_name=""
  files_over_300=0

  echo "Core Implementation Files:"
  echo "--------------------------"

  for file in "$SRC_DIR"/*.c; do
      [ -f "$file" ] || continue
      basename=$(basename "$file")

      # Skip third-party files
      if echo "$basename" | grep -qE "(jsmn|cJSON|_utils\.c$)"; then
          continue
      fi

      meaningful=$(count_meaningful_lines "$file")
      actual=$(wc -l < "$file" | tr -d ' ')
      savings=$((actual - meaningful))
      percent=$((savings * 100 / actual))

      if [ "$meaningful" -gt "$max_file_size" ]; then
          max_file_size=$meaningful
          max_file_name=$basename
      fi
      if [ "$meaningful" -gt 300 ]; then
          files_over_300=$((files_over_300 + 1))
      fi

      printf "  %-30s %5d lines (%d actual, -%d%% diet)\n" \
             "$basename" "$meaningful" "$actual" "$percent"
      total=$((total + meaningful))
  done

  budget=10000
  remaining=$((budget - total))
  percent_used=$((total * 100 / budget))

  echo ""
  echo "Budget Status:"
  echo "--------------"
  printf "  Budget:                        10,000 lines\n"
  printf "  Used:                           %5d lines (%d%%)\n" "$total" "$percent_used"

  if [ $remaining -gt 0 ]; then
      printf "  Remaining:                      %5d lines\n" "$remaining"
  else
      printf "  OVER BUDGET:                    %5d lines\n" "$((remaining * -1))"
  fi

  echo ""
  echo "Complexity Indicators:"
  echo "----------------------"
  file_count=$(find "$SRC_DIR" -name "*.c" ! -name "*jsmn*" ! -name "*cJSON*" | wc -l | tr -d ' ')
  avg_size=$((total / file_count))
  printf "  Total files:                     %3d files\n" "$file_count"
  printf "  Average file size:               %3d lines\n" "$avg_size"
  printf "  Largest file:                    %3d lines (%s)\n" "$max_file_size" "$max_file_name"
  printf "  Files over 300 lines:            %3d files\n" "$files_over_300"
  echo ""

  Make executable:
  chmod +x scripts/dev/count_core.sh

  2. Programming Guidelines Checker

  Argo has a comprehensive 1232-line guidelines checker with 39 automated checks. For Katra:

  Option A - Copy from Argo and adapt:
  # Copy Argo's script
  cp /path/to/argo/scripts/programming_guidelines.sh scripts/
  chmod +x scripts/programming_guidelines.sh

  # Edit to replace:
  # - "argo" → "katra"
  # - "ARGO_" → "KATRA_"
  # - "argo_report_error" → "katra_report_error"

  Option B - Start with essential subset (minimum viable):
  #!/bin/bash
  # © 2025 Casey Koons All rights reserved
  # Programming Guidelines Checker for Katra

  REPORT_FILE="${1:-/tmp/katra_programming_guidelines_report.txt}"
  exec > >(tee "$REPORT_FILE")

  ERRORS=0
  WARNINGS=0

  echo "=========================================="
  echo "KATRA PROGRAMMING GUIDELINES REPORT"
  echo "Generated: $(date '+%Y-%m-%d %H:%M:%S')"
  echo "=========================================="
  echo ""

  # 1. Magic numbers check
  MAGIC_NUMBERS=$(grep -rn '\b[0-9]\{2,\}\b' src/ --include="*.c" | \
    grep -v "//" | grep -v "\*" | grep -v "#" | wc -l)
  echo "1. Magic Numbers: $MAGIC_NUMBERS"
  [ "$MAGIC_NUMBERS" -gt 50 ] && WARNINGS=$((WARNINGS + 1))

  # 2. Unsafe string functions
  UNSAFE_FUNCS=$(grep -rn '\bstrcpy\b\|\bstrcat\b\|\bsprintf\b\|\bgets\b' src/ \
    --include="*.c" | grep -v "strncpy\|strncat\|snprintf" | wc -l)
  echo "2. Unsafe String Functions: $UNSAFE_FUNCS"
  [ "$UNSAFE_FUNCS" -gt 0 ] && ERRORS=$((ERRORS + 1))

  # 3. File size check (max 618 lines)
  LARGE_FILES=$(find src/ -name "*.c" -exec sh -c 'wc -l < "{}" | \
    awk "{if (\$1 > 618) print \"{}\"}"' \; | wc -l)
  echo "3. Files Over 618 Lines: $LARGE_FILES"
  [ "$LARGE_FILES" -gt 0 ] && WARNINGS=$((WARNINGS + 1))

  # 4. Copyright headers
  MISSING_COPYRIGHT=$(find src/ include/ -name "*.c" -o -name "*.h" | \
    xargs grep -L "© 2025 Casey Koons" | wc -l)
  echo "4. Missing Copyright: $MISSING_COPYRIGHT files"
  [ "$MISSING_COPYRIGHT" -gt 0 ] && WARNINGS=$((WARNINGS + 1))

  # 5. Compilation check
  if make clean > /dev/null 2>&1 && make > /dev/null 2>&1; then
      echo "5. Compilation: PASS"
  else
      echo "5. Compilation: FAIL"
      ERRORS=$((ERRORS + 1))
  fi

  # 6. Test suite
  if make test > /dev/null 2>&1; then
      echo "6. Tests: PASS"
  else
      echo "6. Tests: FAIL"
      ERRORS=$((ERRORS + 1))
  fi

  echo ""
  echo "=========================================="
  echo "SUMMARY"
  echo "=========================================="
  echo "Errors:   $ERRORS"
  echo "Warnings: $WARNINGS"

  [ "$ERRORS" -gt 0 ] && exit 1 || exit 0

  Argo's Full Checker Validates:
  - Magic numbers, unsafe functions, file sizes, copyrights
  - Error reporting patterns, TODO comments, memory patterns
  - Compilation, tests, code budget, binary size
  - Signal safety, URL hardcoding, input validation
  - Environment sanitization, timeouts, thread safety
  - Return value checking, file descriptor leaks, NULL checks
  - Function complexity, string externalization, resource cleanup
  - Memory initialization, buffer validation, defensive programming
  - Error path coverage, mutex balance, include guards
  - Error consistency, string allocation, conversion safety
  - Process execution, switch completeness, file I/O errors
  - Function length limits, inline usage

  Run:
  ./scripts/programming_guidelines.sh
  # Report at: /tmp/katra_programming_guidelines_report.txt

  3. Actual Argo Makefile Structure

  Argo uses a modular Makefile approach. Create for Katra:

  Makefile (main orchestrator):
  # © 2025 Casey Koons All rights reserved
  # Katra Project Makefile

  # Include configuration
  include Makefile.config

  # Include functional modules
  include Makefile.build
  include Makefile.test
  include Makefile.clean
  include Makefile.help

  # Default target
  all: directories $(KATRA_BINARY)

  # Directories
  directories:
  	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

  # .PHONY declarations
  .PHONY: all directories test clean distclean count guidelines \
          help install uninstall

  # Programming guidelines check
  guidelines:
  	@echo "Running Programming Guidelines Check..."
  	@./scripts/programming_guidelines.sh

  # Line count
  count:
  	@./scripts/dev/count_core.sh

  # Help
  help:
  	@echo "Katra Makefile Targets:"
  	@echo "  make           - Build katra"
  	@echo "  make test      - Run test suite"
  	@echo "  make clean     - Remove build artifacts"
  	@echo "  make count     - Count meaningful lines"
  	@echo "  make guidelines - Run programming guidelines check"
  	@echo "  make install   - Install to ~/.local/bin/"

  Makefile.config:
  # © 2025 Casey Koons All rights reserved
  # Build Configuration

  CC = gcc
  CFLAGS = -Wall -Werror -Wextra -std=c11 -I./include
  LDFLAGS = -lcurl

  SRC_DIR = src
  BUILD_DIR = build
  BIN_DIR = bin
  INCLUDE_DIR = include

  SOURCES = $(wildcard $(SRC_DIR)/katra_*.c)
  OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

  KATRA_BINARY = $(BIN_DIR)/katra

  Makefile.build:
  # © 2025 Casey Koons All rights reserved
  # Build Rules

  $(KATRA_BINARY): $(OBJECTS) | $(BIN_DIR)
  	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
  	@echo "Built: $@"

  $(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
  	$(CC) $(CFLAGS) -c $< -o $@

  $(BUILD_DIR) $(BIN_DIR):
  	mkdir -p $@

  Makefile.test:
  # © 2025 Casey Koons All rights reserved
  # Test Targets

  TEST_SOURCES = $(wildcard tests/test_*.c)
  TEST_BINARIES = $(TEST_SOURCES:tests/%.c=$(BIN_DIR)/tests/%)

  test: $(TEST_BINARIES)
  	@echo "Running tests..."
  	@for test in $(TEST_BINARIES); do \
  		echo "  Running $$test..."; \
  		$$test || exit 1; \
  	done
  	@echo "All tests passed!"

  $(BIN_DIR)/tests/%: tests/%.c $(OBJECTS) | $(BIN_DIR)/tests
  	$(CC) $(CFLAGS) $< $(OBJECTS) -o $@ $(LDFLAGS)

  $(BIN_DIR)/tests:
  	mkdir -p $@

  Makefile.clean:
  # © 2025 Casey Koons All rights reserved
  # Clean Targets

  clean:
  	rm -rf $(BUILD_DIR) $(BIN_DIR)
  	@echo "Cleaned build artifacts"

  distclean: clean
  	rm -f /tmp/katra_*.txt
  	@echo "Cleaned all generated files"

  4. Pre-Commit Workflow

  Add this to .git/hooks/pre-commit:
  #!/bin/bash
  # © 2025 Casey Koons All rights reserved
  # Pre-commit hook for Katra

  echo "Running pre-commit checks..."

  # 1. Line count check
  echo "1. Checking line count budget..."
  if ! ./scripts/dev/count_core.sh | grep -q "Remaining:"; then
      echo "ERROR: Over line count budget!"
      exit 1
  fi

  # 2. Programming guidelines
  echo "2. Checking programming guidelines..."
  if ! ./scripts/programming_guidelines.sh > /dev/null 2>&1; then
      echo "ERROR: Programming guidelines check failed!"
      echo "Run: ./scripts/programming_guidelines.sh for details"
      exit 1
  fi

  # 3. Build check
  echo "3. Checking compilation..."
  if ! make clean > /dev/null 2>&1 || ! make > /dev/null 2>&1; then
      echo "ERROR: Code does not compile!"
      exit 1
  fi

  # 4. Test check
  echo "4. Running tests..."
  if ! make test > /dev/null 2>&1; then
      echo "ERROR: Tests failing!"
      exit 1
  fi

  echo "All pre-commit checks passed!"
  exit 0

  Make executable:
  chmod +x .git/hooks/pre-commit

  ---
  Key Takeaways for Katra

  1. Use Argo's count_core.sh - Exact script shown above, just change "argo" to "katra"
  2. Adapt programming_guidelines.sh - Start with essential checks (6 shown), expand to full 39 checks
  from Argo
  3. Use modular Makefile - Split into Config, Build, Test, Clean modules like Argo
  4. Run before every commit:
  make clean && make && make test && make count && make guidelines
  5. Budget tracking:
    - Argo started at 10K lines, now at ~16K (multi-provider system)
    - Katra: Start at 10K budget, monitor with ./scripts/dev/count_core.sh
  6. Guidelines as CI - Argo's programming_guidelines.sh is effectively continuous integration in a
  script

  This is the battle-tested approach from Argo. Copy these scripts exactly, adapt the names, and you'll
   have the same code discipline.

