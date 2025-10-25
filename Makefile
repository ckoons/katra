# Makefile
# © 2025 Casey Koons All rights reserved
# Katra Project Makefile

# NOTE: This is a single-file Makefile organized into sections.
# When it grows beyond ~300 lines, we'll split into:
#   Makefile.config, Makefile.build, Makefile.test,
#   Makefile.install, Makefile.clean, Makefile.help

# ==============================================================================
# CONFIG SECTION (future Makefile.config)
# ==============================================================================

# Project
PROJECT_NAME := katra
VERSION := 0.1.0

# Directories
SRC_DIR := src
INCLUDE_DIR := include
BUILD_DIR := build
BIN_DIR := bin
TEST_DIR := tests
SCRIPTS_DIR := scripts

# Compiler and flags
CC := gcc
CFLAGS := -Wall -Werror -Wextra -std=c11 -I$(INCLUDE_DIR)
CFLAGS_DEBUG := $(CFLAGS) -g -O0 -DDEBUG
CFLAGS_RELEASE := $(CFLAGS) -O2 -DNDEBUG

# Linker flags
LDFLAGS :=
LIBS :=

# Tools
MKDIR_P := mkdir -p
RM_RF := rm -rf

# ==============================================================================
# PHONY TARGETS
# ==============================================================================

.PHONY: all clean distclean test help
.PHONY: count-report programming-guidelines check
.PHONY: directories

# ==============================================================================
# DEFAULT TARGET
# ==============================================================================

all: directories $(LIBKATRA_FOUNDATION)
	@echo ""
	@echo "========================================"
	@echo "Katra foundation build complete!"
	@echo "========================================"
	@echo ""
	@echo "Available targets:"
	@echo "  make test-quick             - Run foundation unit tests"
	@echo "  make count-report           - Run line count (diet-aware)"
	@echo "  make programming-guidelines - Run code discipline checks"
	@echo "  make check                  - Run both reports"
	@echo "  make help                   - Show all available targets"

# ==============================================================================
# BUILD SECTION (future Makefile.build)
# ==============================================================================

# Foundation object files
FOUNDATION_OBJS := $(BUILD_DIR)/katra_error.o \
                   $(BUILD_DIR)/katra_log.o \
                   $(BUILD_DIR)/katra_env_utils.o \
                   $(BUILD_DIR)/katra_env_load.o \
                   $(BUILD_DIR)/katra_config.o \
                   $(BUILD_DIR)/katra_init.o

# Memory/Core object files
CORE_OBJS := $(BUILD_DIR)/katra_memory.o \
             $(BUILD_DIR)/katra_tier1.o

# Foundation library
LIBKATRA_FOUNDATION := $(BUILD_DIR)/libkatra_foundation.a

directories:
	@$(MKDIR_P) $(BUILD_DIR)
	@$(MKDIR_P) $(BIN_DIR)
	@$(MKDIR_P) $(BIN_DIR)/tests

# Build foundation library
$(LIBKATRA_FOUNDATION): $(FOUNDATION_OBJS) $(CORE_OBJS)
	@echo "Creating foundation library: $@"
	@ar rcs $@ $^

# Compile foundation sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/foundation/%.c
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS_DEBUG) -c $< -o $@

# Compile core sources
$(BUILD_DIR)/katra_memory.o: $(SRC_DIR)/core/katra_memory.c
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS_DEBUG) -c $< -o $@

$(BUILD_DIR)/katra_tier1.o: $(SRC_DIR)/core/katra_tier1.c
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS_DEBUG) -c $< -o $@

# ==============================================================================
# TEST SECTION (future Makefile.test)
# ==============================================================================

# Test executables
TEST_ENV := $(BIN_DIR)/tests/test_env
TEST_CONFIG := $(BIN_DIR)/tests/test_config
TEST_ERROR := $(BIN_DIR)/tests/test_error
TEST_LOG := $(BIN_DIR)/tests/test_log
TEST_INIT := $(BIN_DIR)/tests/test_init
TEST_MEMORY := $(BIN_DIR)/tests/test_memory
TEST_TIER1 := $(BIN_DIR)/tests/test_tier1

# Test targets
test-quick: test-env test-config test-error test-log test-init test-memory test-tier1
	@echo ""
	@echo "========================================"
	@echo "All foundation and memory tests passed!"
	@echo "========================================"

test: test-quick

test-all: test-quick

# Individual test targets
test-env: $(TEST_ENV)
	@echo "Running environment tests..."
	@$(TEST_ENV)

test-config: $(TEST_CONFIG)
	@echo "Running configuration tests..."
	@$(TEST_CONFIG)

test-error: $(TEST_ERROR)
	@echo "Running error tests..."
	@$(TEST_ERROR)

test-log: $(TEST_LOG)
	@echo "Running logging tests..."
	@$(TEST_LOG)

test-init: $(TEST_INIT)
	@echo "Running initialization tests..."
	@$(TEST_INIT)

test-memory: $(TEST_MEMORY)
	@echo "Running memory tests..."
	@$(TEST_MEMORY)

test-tier1: $(TEST_TIER1)
	@echo "Running Tier 1 storage tests..."
	@$(TEST_TIER1)

# Build test executables
$(TEST_ENV): $(TEST_DIR)/unit/test_env.c $(LIBKATRA_FOUNDATION)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_foundation -lpthread

$(TEST_CONFIG): $(TEST_DIR)/unit/test_config.c $(LIBKATRA_FOUNDATION)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_foundation -lpthread

$(TEST_ERROR): $(TEST_DIR)/unit/test_error.c $(LIBKATRA_FOUNDATION)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_foundation -lpthread

$(TEST_LOG): $(TEST_DIR)/unit/test_log.c $(LIBKATRA_FOUNDATION)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_foundation -lpthread

$(TEST_INIT): $(TEST_DIR)/unit/test_init.c $(LIBKATRA_FOUNDATION)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_foundation -lpthread

$(TEST_MEMORY): $(TEST_DIR)/unit/test_memory.c $(LIBKATRA_FOUNDATION)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_foundation -lpthread

$(TEST_TIER1): $(TEST_DIR)/unit/test_tier1.c $(LIBKATRA_FOUNDATION)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_foundation -lpthread

# ==============================================================================
# CODE DISCIPLINE TARGETS
# ==============================================================================

count-report:
	@echo "Running Katra line count (diet-aware)..."
	@if [ -f $(SCRIPTS_DIR)/dev/count_core.sh ]; then \
		$(SCRIPTS_DIR)/dev/count_core.sh; \
	else \
		echo "Error: $(SCRIPTS_DIR)/dev/count_core.sh not found"; \
		echo "Run: cp from Argo or create script"; \
		exit 1; \
	fi

programming-guidelines:
	@echo "Running Katra programming guidelines check (39 checks)..."
	@if [ -f $(SCRIPTS_DIR)/programming_guidelines.sh ]; then \
		$(SCRIPTS_DIR)/programming_guidelines.sh; \
	else \
		echo "Error: $(SCRIPTS_DIR)/programming_guidelines.sh not found"; \
		echo "Run: cp from Argo or create script"; \
		exit 1; \
	fi

check: programming-guidelines count-report
	@echo ""
	@echo "========================================"
	@echo "All code discipline checks complete"
	@echo "========================================"

# ==============================================================================
# INSTALL SECTION (future Makefile.install)
# ==============================================================================

install:
	@echo "Install target not yet implemented"
	@echo "Will install to ~/.local/bin/ when binaries exist"

uninstall:
	@echo "Uninstall target not yet implemented"

# ==============================================================================
# CLEAN SECTION (future Makefile.clean)
# ==============================================================================

clean:
	@echo "Cleaning build artifacts..."
	@$(RM_RF) $(BUILD_DIR)
	@$(RM_RF) $(BIN_DIR)
	@echo "Clean complete"

distclean: clean
	@echo "Removing all generated files..."
	@$(RM_RF) .env.katra.local
	@$(RM_RF) *.log
	@echo "Distclean complete"

# ==============================================================================
# HELP SECTION (future Makefile.help)
# ==============================================================================

help:
	@echo "Katra Makefile Targets"
	@echo "======================"
	@echo ""
	@echo "Build Targets:"
	@echo "  make           - Build katra (not yet implemented)"
	@echo "  make all       - Same as 'make' (default target)"
	@echo "  make directories - Create build directories"
	@echo ""
	@echo "Code Discipline Targets:"
	@echo "  make count-report           - Count meaningful lines (diet-aware)"
	@echo "  make programming-guidelines - Run 39 code quality checks"
	@echo "  make check                  - Run both discipline reports"
	@echo ""
	@echo "Test Targets:"
	@echo "  make test       - Run test suite (not yet implemented)"
	@echo "  make test-quick - Run quick tests (not yet implemented)"
	@echo "  make test-all   - Run all tests (not yet implemented)"
	@echo ""
	@echo "Install Targets:"
	@echo "  make install   - Install to ~/.local/bin/ (not yet implemented)"
	@echo "  make uninstall - Remove from ~/.local/bin/ (not yet implemented)"
	@echo ""
	@echo "Clean Targets:"
	@echo "  make clean     - Remove build artifacts (build/, bin/)"
	@echo "  make distclean - Remove all generated files"
	@echo ""
	@echo "Help:"
	@echo "  make help      - Show this help message"
	@echo ""
	@echo "Project Info:"
	@echo "  Name:    $(PROJECT_NAME)"
	@echo "  Version: $(VERSION)"
	@echo "  Budget:  10,000 meaningful lines"
	@echo ""
	@echo "Code Discipline:"
	@echo "  - Memory safety: goto cleanup pattern, NULL checks"
	@echo "  - String safety: NO strcpy/sprintf/strcat"
	@echo "  - All constants in headers, none in .c files"
	@echo "  - Max 600 lines per .c file (3% tolerance = 618)"
	@echo "  - Compile: gcc -Wall -Werror -Wextra -std=c11"
	@echo ""
	@echo "See CLAUDE.md for complete coding standards"
