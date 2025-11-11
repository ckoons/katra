# Makefile
# © 2025 Casey Koons All rights reserved
# Katra Project Makefile - Improved version with automatic dependency tracking

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
# Auto-dependency generation: -MMD creates .d files, -MP adds phony targets for headers
CFLAGS_DEBUG := $(CFLAGS) -g -O0 -DDEBUG -MMD -MP
CFLAGS_RELEASE := $(CFLAGS) -O2 -DNDEBUG -MMD -MP

# Linker flags
LDFLAGS :=
LIBS := -lcurl

# Tools
MKDIR_P := mkdir -p
RM_RF := rm -rf

# ==============================================================================
# BUILD VARIABLES (must be before targets that use them)
# ==============================================================================

# Foundation object files
FOUNDATION_OBJS := $(BUILD_DIR)/katra_error.o \
                   $(BUILD_DIR)/katra_log.o \
                   $(BUILD_DIR)/katra_env_utils.o \
                   $(BUILD_DIR)/katra_env_load.o \
                   $(BUILD_DIR)/katra_env_parse.o \
                   $(BUILD_DIR)/katra_config.o \
                   $(BUILD_DIR)/katra_init.o \
                   $(BUILD_DIR)/katra_path_utils.o \
                   $(BUILD_DIR)/katra_json_utils.o \
                   $(BUILD_DIR)/katra_file_utils.o \
                   $(BUILD_DIR)/katra_core_common.o

# Memory/Core object files
CORE_OBJS := $(BUILD_DIR)/katra_memory.o \
             $(BUILD_DIR)/katra_memory_metacognitive.o \
             $(BUILD_DIR)/katra_memory_related.o \
             $(BUILD_DIR)/katra_memory_graph.o \
             $(BUILD_DIR)/katra_tier1.o \
             $(BUILD_DIR)/katra_tier1_json.o \
             $(BUILD_DIR)/katra_tier1_archive.o \
             $(BUILD_DIR)/katra_tier1_pattern.o \
             $(BUILD_DIR)/katra_tier2.o \
             $(BUILD_DIR)/katra_tier2_json.o \
             $(BUILD_DIR)/katra_tier2_index.o \
             $(BUILD_DIR)/katra_tier2_index_mgmt.o \
             $(BUILD_DIR)/katra_consent.o \
             $(BUILD_DIR)/katra_checkpoint.o \
             $(BUILD_DIR)/katra_checkpoint_mgmt.o \
             $(BUILD_DIR)/katra_continuity.o \
             $(BUILD_DIR)/katra_sunrise_sunset.o \
             $(BUILD_DIR)/katra_identity.o \
             $(BUILD_DIR)/katra_phase1_migration.o \
             $(BUILD_DIR)/katra_convergence.o \
             $(BUILD_DIR)/katra_consolidation.o

# Database backend object files
DB_OBJS := $(BUILD_DIR)/katra_db_backend.o \
           $(BUILD_DIR)/katra_db_jsonl.o \
           $(BUILD_DIR)/katra_db_sqlite.o \
           $(BUILD_DIR)/katra_encoder.o \
           $(BUILD_DIR)/katra_vector.o \
           $(BUILD_DIR)/katra_vector_persist.o \
           $(BUILD_DIR)/katra_vector_tfidf.o \
           $(BUILD_DIR)/katra_vector_external.o \
           $(BUILD_DIR)/katra_vector_hnsw.o \
           $(BUILD_DIR)/katra_graph.o \
           $(BUILD_DIR)/katra_tier1_index.o \
           $(BUILD_DIR)/katra_tier1_index_mgmt.o

# Engram/cognitive object files
ENGRAM_OBJS := $(BUILD_DIR)/cognitive_workflows.o \
               $(BUILD_DIR)/emotional_context.o \
               $(BUILD_DIR)/working_memory.o \
               $(BUILD_DIR)/interstitial.o \
               $(BUILD_DIR)/string_utils.o

# Breathing layer object files (Level 2 + Level 3)
BREATHING_OBJS := $(BUILD_DIR)/katra_breathing.o \
                  $(BUILD_DIR)/katra_breathing_primitives.o \
                  $(BUILD_DIR)/katra_breathing_semantic.o \
                  $(BUILD_DIR)/katra_breathing_context.o \
                  $(BUILD_DIR)/katra_breathing_context_persist.o \
                  $(BUILD_DIR)/katra_breathing_context_update.o \
                  $(BUILD_DIR)/katra_breathing_context_capture.o \
                  $(BUILD_DIR)/katra_breathing_context_query.o \
                  $(BUILD_DIR)/katra_breathing_config.o \
                  $(BUILD_DIR)/katra_breathing_interstitial.o \
                  $(BUILD_DIR)/katra_breathing_integration.o \
                  $(BUILD_DIR)/katra_breathing_helpers.o \
                  $(BUILD_DIR)/katra_breathing_health.o \
                  $(BUILD_DIR)/katra_breathing_reflection.o

# Chat (Inter-CI Communication) object files
CHAT_OBJS := $(BUILD_DIR)/katra_chat.o \
             $(BUILD_DIR)/katra_chat_helpers.o \
             $(BUILD_DIR)/katra_chat_registry.o

# Lifecycle (Autonomic Breathing) object files
LIFECYCLE_OBJS := $(BUILD_DIR)/katra_lifecycle.o

# Hook Adapter object files (Phase 3)
HOOKS_OBJS := $(BUILD_DIR)/hook_registry.o \
              $(BUILD_DIR)/hook_anthropic.o

# Phase 5 object files (Memory-Augmented Reasoning)
NOUS_OBJS := $(BUILD_DIR)/katra_nous_common.o \
               $(BUILD_DIR)/katra_nous_compose.o \
               $(BUILD_DIR)/katra_nous_patterns.o \
               $(BUILD_DIR)/katra_nous_impact.o \
               $(BUILD_DIR)/katra_nous_reasoning.o \
               $(BUILD_DIR)/katra_nous_crossproject.o

# MCP library object files (without server main)
MCP_LIB_OBJS := $(BUILD_DIR)/mcp_protocol.o \
                $(BUILD_DIR)/mcp_tools_memory.o \
                $(BUILD_DIR)/mcp_tools_identity.o \
                $(BUILD_DIR)/mcp_nous.o \
                $(BUILD_DIR)/mcp_resources.o

# MCP Server object files (includes server main)
MCP_OBJS := $(MCP_LIB_OBJS) \
            $(BUILD_DIR)/katra_mcp_server.o

# Utils library
LIBKATRA_UTILS := $(BUILD_DIR)/libkatra_utils.a

# MCP Server binary
MCP_SERVER := $(BIN_DIR)/katra_mcp_server

# Jansson library flags (for MCP server JSON-RPC)
JANSSON_CFLAGS := -I/opt/homebrew/Cellar/jansson/2.14.1/include
JANSSON_LDFLAGS := -L/opt/homebrew/Cellar/jansson/2.14.1/lib -ljansson

# All object files (for dependency tracking)
ALL_OBJS := $(FOUNDATION_OBJS) $(CORE_OBJS) $(DB_OBJS) $(ENGRAM_OBJS) $(BREATHING_OBJS) $(CHAT_OBJS) $(LIFECYCLE_OBJS) $(HOOKS_OBJS) $(NOUS_OBJS) $(MCP_OBJS)

# Auto-generated dependency files
DEP_FILES := $(ALL_OBJS:.o=.d)

# ==============================================================================
# PHONY TARGETS
# ==============================================================================

.PHONY: all clean clean-all distclean test help
.PHONY: count-report programming-guidelines check
.PHONY: improvement-scan benchmark benchmark-reflection check-ready
.PHONY: directories mcp-server
.PHONY: test-quick test-env test-config test-error test-log test-init test-memory
.PHONY: test-tier1 test-tier2 test-tier2-index test-checkpoint test-continuity
.PHONY: test-vector test-graph test-sunrise-sunset test-consent test-corruption
.PHONY: test-lifecycle test-mock-ci test-breathing-phase2 test-breathing-primitives
.PHONY: test-breathing-enhancements test-session-info test-context-persist test-mcp
.PHONY: test-turn-tracking test-reflection-integration

# ==============================================================================
# DEFAULT TARGET
# ==============================================================================

all: directories $(LIBKATRA_UTILS) $(MCP_SERVER)
	@echo ""
	@echo "========================================"
	@echo "Katra build complete!"
	@echo "========================================"
	@echo ""
	@echo "Built targets:"
	@echo "  Utils library: $(LIBKATRA_UTILS)"
	@echo "  MCP Server:    $(MCP_SERVER)"
	@echo ""
	@echo "Available targets:"
	@echo "  make test-quick             - Run all tests (including MCP)"
	@echo "  make mcp-server             - Build MCP server only"
	@echo "  make count-report           - Run line count (diet-aware)"
	@echo "  make programming-guidelines - Run code discipline checks"
	@echo "  make check                  - Run both reports"
	@echo "  make help                   - Show all available targets"

# ==============================================================================
# BUILD SECTION (future Makefile.build)
# ==============================================================================

# Create directories (order-only prerequisite to avoid timestamp issues)
directories:
	@$(MKDIR_P) $(BUILD_DIR)
	@$(MKDIR_P) $(BIN_DIR)
	@$(MKDIR_P) $(BIN_DIR)/tests

# Pattern rule to create build directory (for order-only prerequisites)
$(BUILD_DIR):
	@$(MKDIR_P) $@

# Build utils library
# Note: Delete library first to ensure deterministic rebuild
$(LIBKATRA_UTILS): $(FOUNDATION_OBJS) $(CORE_OBJS) $(DB_OBJS) $(ENGRAM_OBJS) $(BREATHING_OBJS) $(CHAT_OBJS) $(LIFECYCLE_OBJS) $(HOOKS_OBJS) $(NOUS_OBJS)
	@echo "Creating utils library: $@"
	@rm -f $@
	@ar rcs $@ $^
	@echo "Library created successfully"

# ==============================================================================
# PATTERN RULES - Automatic compilation rules per directory
# ==============================================================================
# These replace the 30+ explicit rules with 5 concise pattern rules
# The '| $(BUILD_DIR)' is an order-only prerequisite - creates dir without
# triggering rebuilds when dir timestamp changes

# Compile foundation sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/utils/%.c | $(BUILD_DIR)
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS_DEBUG) -c $< -o $@

# Compile core sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/core/%.c | $(BUILD_DIR)
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS_DEBUG) -c $< -o $@

# Exception: katra_identity.c uses jansson
$(BUILD_DIR)/katra_identity.o: $(SRC_DIR)/core/katra_identity.c | $(BUILD_DIR)
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS_DEBUG) $(JANSSON_CFLAGS) -c $< -o $@

# Compile DB backend sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/db/%.c | $(BUILD_DIR)
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS_DEBUG) -c $< -o $@

# Compile engram sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/psyche/%.c | $(BUILD_DIR)
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS_DEBUG) -c $< -o $@

# Compile breathing layer sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/breathing/%.c | $(BUILD_DIR)
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS_DEBUG) -c $< -o $@

# Compile chat sources (needs Jansson for MCP integration)
$(BUILD_DIR)/%.o: $(SRC_DIR)/chat/%.c | $(BUILD_DIR)
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS_DEBUG) $(JANSSON_CFLAGS) -c $< -o $@

# Compile lifecycle sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/lifecycle/%.c | $(BUILD_DIR)
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS_DEBUG) -c $< -o $@

# Compile hook adapter sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/hooks/%.c | $(BUILD_DIR)
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS_DEBUG) -c $< -o $@

# Compile Phase 5 sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/nous/%.c | $(BUILD_DIR)
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS_DEBUG) -c $< -o $@

# Compile MCP sources (requires jansson headers)
$(BUILD_DIR)/%.o: $(SRC_DIR)/mcp/%.c | $(BUILD_DIR)
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS_DEBUG) $(JANSSON_CFLAGS) -c $< -o $@

# ==============================================================================
# MCP SERVER BUILD
# ==============================================================================

# Build MCP server binary
$(MCP_SERVER): $(MCP_OBJS) $(LIBKATRA_UTILS) | $(BIN_DIR)
	@echo "Building MCP server: $@"
	@$(CC) -o $@ $(MCP_OBJS) -L$(BUILD_DIR) -lkatra_utils $(JANSSON_LDFLAGS) -lsqlite3 -lpthread $(LIBS)
	@echo "Copying wrapper script to bin/..."
	@cp $(SCRIPTS_DIR)/katra_mcp_server_wrapper.sh $(BIN_DIR)/
	@chmod +x $(BIN_DIR)/katra_mcp_server_wrapper.sh
	@echo "MCP server built successfully"

# Convenience target
mcp-server: $(MCP_SERVER)

# ==============================================================================
# AUTOMATIC DEPENDENCY INCLUSION
# ==============================================================================
# Include auto-generated .d files (header dependencies)
# The '-' prefix means "don't error if files don't exist yet"
# Make will automatically regenerate them during compilation
-include $(DEP_FILES)

# Mark .d files as secondary targets (don't delete them as intermediate files)
.SECONDARY: $(DEP_FILES)

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
TEST_TIER2 := $(BIN_DIR)/tests/test_tier2
TEST_TIER2_INDEX := $(BIN_DIR)/tests/test_tier2_index
TEST_CHECKPOINT := $(BIN_DIR)/tests/test_checkpoint
TEST_CONTINUITY := $(BIN_DIR)/tests/test_continuity
TEST_VECTOR := $(BIN_DIR)/tests/test_vector
TEST_VECTOR_PERSIST := $(BIN_DIR)/tests/test_vector_persist
TEST_VECTOR_TFIDF := $(BIN_DIR)/tests/test_vector_tfidf
TEST_VECTOR_EXTERNAL := $(BIN_DIR)/tests/test_vector_external
TEST_VECTOR_HNSW := $(BIN_DIR)/tests/test_vector_hnsw
TEST_GRAPH := $(BIN_DIR)/tests/test_graph
TEST_SUNRISE_SUNSET := $(BIN_DIR)/tests/test_sunrise_sunset

# New test categories
TEST_CONSENT := $(BIN_DIR)/tests/test_consent_enforcement
TEST_CORRUPTION := $(BIN_DIR)/tests/test_corruption_recovery
TEST_LIFECYCLE := $(BIN_DIR)/tests/test_memory_lifecycle
TEST_MOCK_CI := $(BIN_DIR)/tests/test_mock_ci
TEST_BREATHING_PHASE2 := $(BIN_DIR)/tests/test_breathing_phase2
TEST_BREATHING_PRIMITIVES := $(BIN_DIR)/tests/test_breathing_primitives
TEST_BREATHING_ENHANCEMENTS := $(BIN_DIR)/tests/test_breathing_enhancements
TEST_SESSION_INFO := $(BIN_DIR)/tests/test_session_info
TEST_CONTEXT_PERSIST := $(BIN_DIR)/tests/test_context_persist
TEST_MCP := $(BIN_DIR)/tests/test_mcp
TEST_TURN_TRACKING := $(BIN_DIR)/tests/test_turn_tracking
TEST_REFLECTION_INTEGRATION := $(BIN_DIR)/tests/test_reflection_integration

# Benchmark executables
BENCHMARK_TIER2_QUERY := $(BIN_DIR)/benchmark_tier2_query
BENCHMARK_REFLECTION := $(BIN_DIR)/tests/benchmark_reflection

# Test targets
test-quick: test-env test-config test-error test-log test-init test-memory test-tier1 test-tier2 test-tier2-index test-checkpoint test-continuity test-vector test-vector-persist test-vector-tfidf test-vector-external test-vector-hnsw test-graph test-sunrise-sunset test-consent test-corruption test-lifecycle test-mock-ci test-breathing-phase2 test-breathing-primitives test-breathing-enhancements test-session-info test-context-persist test-mcp test-turn-tracking test-reflection-integration
	@echo ""
	@echo "========================================"
	@echo "All tests passed!"
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

test-tier2: $(TEST_TIER2)
	@echo "Running Tier 2 storage tests..."
	@$(TEST_TIER2)

test-tier2-index: $(TEST_TIER2_INDEX)
	@echo "Running Tier 2 index tests..."
	@$(TEST_TIER2_INDEX)

test-checkpoint: $(TEST_CHECKPOINT)
	@echo "Running checkpoint tests..."
	@$(TEST_CHECKPOINT)

test-continuity: $(TEST_CONTINUITY)
	@echo "Running continuity tests..."
	@$(TEST_CONTINUITY)

test-vector: $(TEST_VECTOR)
	@echo "Running vector database tests..."
	@$(TEST_VECTOR)

test-vector-persist: $(TEST_VECTOR_PERSIST)
	@echo "Running vector persistence tests..."
	@$(TEST_VECTOR_PERSIST)

test-vector-tfidf: $(TEST_VECTOR_TFIDF)
	@echo "Running TF-IDF embedding tests..."
	@$(TEST_VECTOR_TFIDF)

test-vector-external: $(TEST_VECTOR_EXTERNAL)
	@echo "Running external embedding tests..."
	@$(TEST_VECTOR_EXTERNAL)

test-vector-hnsw: $(TEST_VECTOR_HNSW)
	@echo "Running HNSW indexing tests..."
	@$(TEST_VECTOR_HNSW)

test-graph: $(TEST_GRAPH)
	@echo "Running graph database tests..."
	@$(TEST_GRAPH)

test-sunrise-sunset: $(TEST_SUNRISE_SUNSET)
	@echo "Running sunrise/sunset tests..."
	@$(TEST_SUNRISE_SUNSET)

test-consent: $(TEST_CONSENT)
	@echo "Running ethical consent tests..."
	@$(TEST_CONSENT)

test-corruption: $(TEST_CORRUPTION)
	@echo "Running corruption recovery tests..."
	@$(TEST_CORRUPTION)

test-lifecycle: $(TEST_LIFECYCLE)
	@echo "Running memory lifecycle integration tests..."
	@$(TEST_LIFECYCLE)

test-mock-ci: $(TEST_MOCK_CI)
	@echo "Running mock CI integration tests..."
	@$(TEST_MOCK_CI)

test-breathing-phase2: $(TEST_BREATHING_PHASE2)
	@echo "Running breathing layer Phase 2 tests..."
	@$(TEST_BREATHING_PHASE2)

test-breathing-primitives: $(TEST_BREATHING_PRIMITIVES)
	@echo "Running breathing primitives unit tests..."
	@$(TEST_BREATHING_PRIMITIVES)

test-breathing-enhancements: $(TEST_BREATHING_ENHANCEMENTS)
	@echo "Running breathing enhancements unit tests..."
	@$(TEST_BREATHING_ENHANCEMENTS)

test-session-info: $(TEST_SESSION_INFO)
	@echo "Running session info API tests..."
	@$(TEST_SESSION_INFO)

test-context-persist: $(TEST_CONTEXT_PERSIST)
	@echo "Running context persistence tests..."
	@$(TEST_CONTEXT_PERSIST)

test-mcp: $(TEST_MCP)
	@echo "Running MCP server tests..."
	@$(TEST_MCP)

test-turn-tracking: $(TEST_TURN_TRACKING)
	@echo "Running turn tracking and metadata tests..."
	@$(TEST_TURN_TRACKING)

test-reflection-integration: $(TEST_REFLECTION_INTEGRATION)
	@echo "Running reflection integration test..."
	@$(TEST_REFLECTION_INTEGRATION)

# CI readiness check
check-ready: directories $(LIBKATRA_UTILS)
	@echo "Checking Katra readiness for CI testing..."
	@./scripts/check_ready.sh

# Build test executables
$(TEST_ENV): $(TEST_DIR)/unit/test_env.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

$(TEST_CONFIG): $(TEST_DIR)/unit/test_config.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

$(TEST_ERROR): $(TEST_DIR)/unit/test_error.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

$(TEST_LOG): $(TEST_DIR)/unit/test_log.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

$(TEST_INIT): $(TEST_DIR)/unit/test_init.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

$(TEST_MEMORY): $(TEST_DIR)/unit/test_memory.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

$(TEST_TIER1): $(TEST_DIR)/unit/test_tier1.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

$(TEST_TIER2): $(TEST_DIR)/unit/test_tier2.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

$(TEST_TIER2_INDEX): $(TEST_DIR)/unit/test_tier2_index.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

$(TEST_CHECKPOINT): $(TEST_DIR)/unit/test_checkpoint.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

$(TEST_CONTINUITY): $(TEST_DIR)/unit/test_continuity.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

$(TEST_VECTOR): $(TEST_DIR)/unit/test_vector.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread -lm

$(TEST_VECTOR_PERSIST): $(TEST_DIR)/test_vector_persist.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread -lm -lcurl

$(TEST_VECTOR_TFIDF): $(TEST_DIR)/test_vector_tfidf.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread -lm -lcurl

$(TEST_VECTOR_EXTERNAL): $(TEST_DIR)/test_vector_external.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread -lm -lcurl

$(TEST_VECTOR_HNSW): $(TEST_DIR)/test_vector_hnsw.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread -lm -lcurl

$(TEST_GRAPH): $(TEST_DIR)/unit/test_graph.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

$(TEST_SUNRISE_SUNSET): $(TEST_DIR)/unit/test_sunrise_sunset.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread -lm

$(TEST_CONSENT): $(TEST_DIR)/ethical/test_consent_enforcement.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

$(TEST_CORRUPTION): $(TEST_DIR)/failure/test_corruption_recovery.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

$(TEST_LIFECYCLE): $(TEST_DIR)/integration/test_memory_lifecycle.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread -lm

$(TEST_MOCK_CI): $(TEST_DIR)/integration/test_mock_ci.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread -lm

$(TEST_BREATHING_PHASE2): $(TEST_DIR)/test_breathing_phase2.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

$(TEST_BREATHING_PRIMITIVES): $(TEST_DIR)/test_breathing_primitives.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

$(TEST_BREATHING_ENHANCEMENTS): $(TEST_DIR)/test_breathing_enhancements.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

$(TEST_SESSION_INFO): $(TEST_DIR)/test_session_info.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

$(TEST_CONTEXT_PERSIST): $(TEST_DIR)/test_context_persist.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

$(TEST_MCP): $(TEST_DIR)/test_mcp.c $(MCP_LIB_OBJS) $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) $(JANSSON_CFLAGS) -o $@ $< $(MCP_LIB_OBJS) -L$(BUILD_DIR) -lkatra_utils $(JANSSON_LDFLAGS) -lsqlite3 -lpthread

$(TEST_TURN_TRACKING): $(TEST_DIR)/test_turn_tracking.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

$(TEST_REFLECTION_INTEGRATION): $(TEST_DIR)/test_reflection_integration.c $(LIBKATRA_UTILS)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

# Benchmark executables
$(BENCHMARK_TIER2_QUERY): $(TEST_DIR)/performance/benchmark_tier2_query.c $(LIBKATRA_UTILS)
	@echo "Building benchmark: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

$(BENCHMARK_REFLECTION): $(TEST_DIR)/benchmark_reflection.c $(LIBKATRA_UTILS)
	@echo "Building benchmark: $@"
	@$(CC) $(CFLAGS_DEBUG) -o $@ $< -L$(BUILD_DIR) -lkatra_utils -lsqlite3 -lpthread

# Benchmark targets
benchmark: $(BENCHMARK_TIER2_QUERY)
	@echo ""
	@echo "Running Tier 2 query performance benchmark..."
	@$(BENCHMARK_TIER2_QUERY)

benchmark-reflection: $(BENCHMARK_REFLECTION)
	@echo ""
	@echo "Running reflection system performance benchmark..."
	@$(BENCHMARK_REFLECTION)

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

improvement-scan:
	@echo "Running Katra improvement scan..."
	@if [ -f $(SCRIPTS_DIR)/scan_improvements.sh ]; then \
		$(SCRIPTS_DIR)/scan_improvements.sh; \
	else \
		echo "Error: $(SCRIPTS_DIR)/scan_improvements.sh not found"; \
		exit 1; \
	fi

# ==============================================================================
# INSTALL SECTION (future Makefile.install)
# ==============================================================================

install:
	@echo "Install target not yet implemented"
	@echo "Will install to ~/.local/bin/ when binaries exist"

uninstall:
	@echo "Uninstall target not yet implemented"

# MCP-specific install and restart targets
install-mcp: mcp-server restart-mcp
	@echo "MCP server installed and restarted"
	@echo "Claude Code will automatically use the new binary on next tool call"

restart-mcp:
	@echo "Restarting MCP server..."
	@pkill -9 -f katra_mcp_server 2>/dev/null || true
	@sleep 1
	@if pgrep -f katra_mcp_server >/dev/null 2>&1; then \
		echo "Warning: Some MCP server processes still running"; \
		pgrep -f katra_mcp_server; \
	else \
		echo "All MCP server processes terminated"; \
		echo "Claude Code will restart from new binary on next tool call"; \
	fi

# Developer tools install targets (Phase 4.5)
INSTALL_DIR ?= $(HOME)/bin
INSTALL_MODE := 0755

install-k:
	@echo "Installing Katra developer tools..."
	@$(MKDIR_P) $(INSTALL_DIR)
	@install -m $(INSTALL_MODE) $(SCRIPTS_DIR)/katra $(INSTALL_DIR)/katra
	@install -m $(INSTALL_MODE) $(SCRIPTS_DIR)/k $(INSTALL_DIR)/k
	@echo "Installed:"
	@echo "  $(INSTALL_DIR)/katra - Start Claude Code with Katra environment"
	@echo "  $(INSTALL_DIR)/k - One-shot CLI queries with Katra"
	@echo ""
	@echo "Make sure $(INSTALL_DIR) is in your PATH"
	@if echo "$$PATH" | grep -q "$(INSTALL_DIR)"; then \
		echo "✓ $(INSTALL_DIR) is already in PATH"; \
	else \
		echo "⚠ Add this to your ~/.bashrc or ~/.zshrc:"; \
		echo "  export PATH=\"$(INSTALL_DIR):\$$PATH\""; \
	fi

uninstall-k:
	@echo "Uninstalling Katra developer tools..."
	@rm -f $(INSTALL_DIR)/katra
	@rm -f $(INSTALL_DIR)/k
	@echo "Uninstalled from $(INSTALL_DIR)"

install-all: install-k
	@echo ""
	@echo "All Katra tools installed successfully"

# ==============================================================================
# CLEAN SECTION (future Makefile.clean)
# ==============================================================================

clean:
	@echo "Cleaning build artifacts..."
	@$(RM_RF) $(BUILD_DIR)
	@$(RM_RF) $(BIN_DIR)
	@echo "Clean complete"

clean-all: clean
	@echo "Deep clean (build/, bin/, logs, temp files)..."
	@$(RM_RF) *.log
	@$(RM_RF) /tmp/katra_*.txt
	@echo "Clean-all complete"

distclean: clean-all
	@echo "Removing all generated and local config files..."
	@$(RM_RF) .env.katra.local
	@$(RM_RF) .katra/
	@echo "Distclean complete (preserves .env.katra)"

# ==============================================================================
# HELP SECTION (future Makefile.help)
# ==============================================================================

help:
	@echo "Katra Makefile Targets"
	@echo "======================"
	@echo ""
	@echo "Build Targets:"
	@echo "  make           - Build katra utils library"
	@echo "  make all       - Same as 'make' (default target)"
	@echo "  make directories - Create build directories"
	@echo ""
	@echo "Code Discipline Targets:"
	@echo "  make count-report           - Count meaningful lines (diet-aware)"
	@echo "  make programming-guidelines - Run 39 code quality checks"
	@echo "  make check                  - Run both discipline reports"
	@echo "  make improvement-scan       - Scan for code improvement opportunities"
	@echo ""
	@echo "Test Targets:"
	@echo "  make test       - Run all tests"
	@echo "  make test-quick - Run quick test suite (21 tests)"
	@echo "  make test-all   - Run all tests (same as test-quick)"
	@echo ""
	@echo "Install Targets:"
	@echo "  make install-k    - Install katra/k scripts to ~/bin (Phase 4.5 tools)"
	@echo "  make uninstall-k  - Uninstall katra/k scripts from ~/bin"
	@echo "  make install-all  - Install all Katra tools (currently install-k)"
	@echo "  make install-mcp  - Rebuild and restart MCP server (kills old processes)"
	@echo "  make restart-mcp  - Kill running MCP servers (auto-restart on next use)"
	@echo ""
	@echo "Clean Targets:"
	@echo "  make clean      - Remove build artifacts (build/, bin/)"
	@echo "  make clean-all  - Deep clean (build/, bin/, logs, temp files)"
	@echo "  make distclean  - Remove all generated + local config"
	@echo ""
	@echo "Help:"
	@echo "  make help      - Show this help message"
	@echo ""
	@echo "Project Info:"
	@echo "  Name:    $(PROJECT_NAME)"
	@echo "  Version: $(VERSION)"
	@echo "  Budget:  10,000 meaningful lines"
	@echo ""
	@echo "Build Improvements (vs old Makefile):"
	@echo "  ✓ Automatic header dependency tracking (-MMD -MP)"
	@echo "  ✓ Pattern rules (5 rules vs 30+ explicit rules)"
	@echo "  ✓ Deterministic library rebuild"
	@echo "  ✓ Parallel build safety (make -j4)"
	@echo "  ✓ Order-only prerequisites for directories"
	@echo ""
	@echo "Code Discipline:"
	@echo "  - Memory safety: goto cleanup pattern, NULL checks"
	@echo "  - String safety: NO strcpy/sprintf/strcat"
	@echo "  - All constants in headers, none in .c files"
	@echo "  - Max 600 lines per .c file (3% tolerance = 618)"
	@echo "  - Compile: gcc -Wall -Werror -Wextra -std=c11"
	@echo ""
	@echo "See CLAUDE.md for complete coding standards"
