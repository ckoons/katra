#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# test_and_store.sh - Run tests and store results in Katra
#
# Usage: ./test_and_store.sh

set -euo pipefail

echo "Running tests..."

# Run tests and capture output
TEST_OUTPUT=$(make test 2>&1)
TEST_STATUS=$?

# Store result in Katra
if [ $TEST_STATUS -eq 0 ]; then
    echo "✅ Tests passed"
    echo "$TEST_OUTPUT" | k --remember "All tests passed on $(date)"
else
    echo "❌ Tests failed"
    echo "$TEST_OUTPUT" | k --remember "Tests failed on $(date) - needs investigation"
fi

exit $TEST_STATUS
