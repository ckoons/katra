# Makefile
# © 2025 Casey Koons All rights reserved
# Katra Project Makefile - Modular build system

# This Makefile is split into modules for maintainability:
#   Makefile.config - Variables and configuration
#   Makefile.build  - Compilation and linking rules
#   Makefile.test   - Test and benchmark targets
#   Makefile        - Main orchestration (this file)

# ==============================================================================
# MODULE INCLUSION
# ==============================================================================

include Makefile.config
include Makefile.build
include Makefile.test

# ==============================================================================
# PHONY TARGETS
# ==============================================================================

.PHONY: all clean help test test-quick mcp-server \
        benchmark benchmark-reflection benchmark-vector \
        count-report programming-guidelines check check-ready improvement-scan \
        install-mcp restart-mcp install-k install-all uninstall-k

# ==============================================================================
# DEFAULT TARGET
# ==============================================================================

all: $(LIBKATRA_UTILS) $(MCP_SERVER)
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
	@echo ""

# ==============================================================================
# CODE DISCIPLINE TARGETS
# ==============================================================================

count-report:
	@echo "Running Katra line count (diet-aware)..."
	@if [ -f $(SCRIPTS_DIR)/dev/count_core.sh ]; then \
		$(SCRIPTS_DIR)/dev/count_core.sh; \
	else \
		echo "Error: count_core.sh not found"; \
		exit 1; \
	fi

programming-guidelines:
	@echo "Running Katra programming guidelines check (39 checks)..."
	@if [ -f $(SCRIPTS_DIR)/programming_guidelines.sh ]; then \
		$(SCRIPTS_DIR)/programming_guidelines.sh; \
	else \
		echo "Error: programming_guidelines.sh not found"; \
		exit 1; \
	fi

check: count-report programming-guidelines
	@echo ""
	@echo "========================================"
	@echo "All checks complete!"
	@echo "========================================"

check-ready:
	@if [ -f $(SCRIPTS_DIR)/check_ready.sh ]; then \
		$(SCRIPTS_DIR)/check_ready.sh; \
	else \
		echo "Error: check_ready.sh not found"; \
		exit 1; \
	fi

improvement-scan:
	@if [ -f $(SCRIPTS_DIR)/scan_improvements.sh ]; then \
		$(SCRIPTS_DIR)/scan_improvements.sh; \
	else \
		echo "Error: scan_improvements.sh not found"; \
		exit 1; \
	fi

# ==============================================================================
# INSTALL TARGET
# ==============================================================================

install: all
	@echo "Installing Katra MCP server..."
	@$(MKDIR_P) ~/.local/bin
	@cp $(MCP_SERVER) ~/.local/bin/
	@cp $(BIN_DIR)/katra_mcp_server_wrapper.sh ~/.local/bin/
	@echo "Katra MCP server installed to ~/.local/bin/"
	@echo "Add ~/.local/bin to your PATH if not already present"

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
		echo "MCP server stopped"; \
	fi

# Install CLI tools (katra and k scripts)
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
# CLEAN TARGETS
# ==============================================================================

clean:
	@echo "Cleaning build artifacts..."
	@$(RM_RF) $(BUILD_DIR)
	@$(RM_RF) $(BIN_DIR)
	@echo "Clean complete"

clean-tests:
	@echo "Cleaning test executables..."
	@$(RM_RF) $(BIN_DIR)/tests
	@echo "Test executables cleaned"

clean-all: clean
	@echo "Deep cleaning (including dependency files)..."
	@find . -name "*.o" -delete
	@find . -name "*.d" -delete
	@find . -name "*.a" -delete
	@echo "Deep clean complete"

# ==============================================================================
# HELP TARGET
# ==============================================================================

help:
	@echo "Katra Makefile - Available targets:"
	@echo ""
	@echo "Build targets:"
	@echo "  make                    - Build all targets (default)"
	@echo "  make all                - Build all targets"
	@echo "  make mcp-server         - Build MCP server only"
	@echo "  make clean              - Remove build artifacts"
	@echo "  make clean-tests        - Remove test executables only"
	@echo "  make clean-all          - Deep clean (including .d files)"
	@echo ""
	@echo "Test targets:"
	@echo "  make test               - Run all tests"
	@echo "  make test-quick         - Run all tests (same as test)"
	@echo "  make test-<name>        - Run specific test (e.g., test-memory)"
	@echo ""
	@echo "Benchmark targets:"
	@echo "  make benchmark          - Run tier2 query benchmark"
	@echo "  make benchmark-reflection - Run reflection system benchmark"
	@echo "  make benchmark-vector   - Run vector search benchmark"
	@echo ""
	@echo "Code discipline:"
	@echo "  make count-report       - Run line count (diet-aware)"
	@echo "  make programming-guidelines - Run code quality checks"
	@echo "  make check              - Run both count and guidelines"
	@echo "  make check-ready        - Check if ready for commit"
	@echo "  make improvement-scan   - Scan for improvement opportunities"
	@echo ""
	@echo "Install:"
	@echo "  make install            - Install to ~/.local/bin"
	@echo "  make install-mcp        - Build MCP server and restart"
	@echo "  make restart-mcp        - Kill and restart MCP server"
	@echo "  make install-k          - Install katra/k CLI tools to ~/bin"
	@echo "  make install-all        - Install all CLI tools"
	@echo "  make uninstall-k        - Uninstall katra/k CLI tools"
	@echo ""
	@echo "Help:"
	@echo "  make help               - Show this help message"
	@echo ""
