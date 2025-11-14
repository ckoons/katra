# Katra Troubleshooting Guide

© 2025 Casey Koons. All rights reserved.

## Introduction

This guide helps you diagnose and resolve common issues with Katra. Whether you're a user experiencing problems, a CI seeking to understand what's happening, or a developer debugging the system, you'll find practical solutions here.

**Organization:**
1. Common Errors (sorted by frequency)
2. Storage Issues
3. Memory Retrieval Problems
4. Checkpoint and Recovery
5. MCP Integration Issues
6. Performance Problems
7. Building and Compilation
8. When to Seek Help

---

## Quick Diagnosis

### Symptoms and Likely Causes

| Symptom | Likely Cause | Section |
|---------|--------------|---------|
| "Memory not found" errors | CI ID mismatch, empty storage | [Memory Retrieval](#memory-retrieval-problems) |
| Compilation fails | Missing dependencies, C11 standard | [Building](#building-and-compilation) |
| MCP server won't start | Path issues, permissions | [MCP Integration](#mcp-integration-issues) |
| Slow recall performance | Large tier1 files, no tier2 index | [Performance](#performance-problems) |
| Checkpoint restore fails | Corrupted checkpoint, wrong CI ID | [Checkpoint Recovery](#checkpoint-and-recovery) |
| Permissions errors | Directory ownership, 0755/0644 modes | [Storage Issues](#storage-issues) |

---

## Common Errors

### Error: E_CONSENT_DENIED (4001)

**Symptom:**
```
[ERROR] Access denied - consent not granted for CI 'Bob' to access memories owned by 'Alice'
```

**Cause:** Attempting to access memories without consent context set.

**Solution:**

**For MCP users:**
```json
{
  "mcpServers": {
    "katra": {
      "env": {
        "KATRA_PERSONA": "YourName"
      }
    }
  }
}
```

**For direct API:**
```c
/* Set consent context before access */
katra_consent_set_context("Alice");

/* Now operations are scoped to Alice's permissions */
katra_memory_query("topic:debugging", &results, &count);
```

**For CIs:** You're seeing this because someone tried to access memories they don't have permission to read. This is working as intended - your privacy is protected.

---

### Error: E_CI_NOT_FOUND (2001)

**Symptom:**
```
[ERROR] CI 'Alice' not found in system
```

**Cause:** CI hasn't created any memories yet, or KATRA_PERSONA doesn't match existing directory.

**Solution:**

**Check if CI exists:**
```bash
ls -la ~/.katra/memory/tier1/ | grep -i alice
```

**If no matches found:**
- CI is new and hasn't stored memories yet (normal)
- First memory operation will create the directory

**If matches found but with different capitalization:**
```bash
# Wrong: KATRA_PERSONA=alice (lowercase)
# Directory: ~/.katra/memory/tier1/Alice/ (uppercase)

# Fix: Match the capitalization exactly
export KATRA_PERSONA=Alice
```

**Key insight:** KATRA_PERSONA must match directory name exactly (case-sensitive).

---

### Error: E_SYSTEM_MEMORY (1001)

**Symptom:**
```
[ERROR] Memory allocation failed
```

**Cause:** System out of memory, or allocation size too large.

**Solution:**

**Check available memory:**
```bash
# macOS
vm_stat | grep free

# Linux
free -h
```

**Check Katra's memory usage:**
```bash
# Find Katra processes
ps aux | grep katra

# Check memory limits
ulimit -a
```

**Common fixes:**
- Close other applications
- Increase system swap space
- Reduce batch query sizes
- Check for memory leaks (if developing)

---

### Error: E_FILE_NOT_FOUND (3002)

**Symptom:**
```
[ERROR] File not found: /Users/user/.katra/memory/tier1/Alice/2025-01-13.jsonl
```

**Cause:** Attempting to read memories from a day that has no recordings.

**Solution:**

**Check what days have recordings:**
```bash
ls -la ~/.katra/memory/tier1/Alice/
```

**This is often normal:** If CI was inactive on that day, there's no file. Katra will create the file when first memory is stored.

**If file should exist:**
- Check disk space: `df -h ~/.katra`
- Check permissions: `ls -la ~/.katra/memory/tier1/Alice/`
- Check for I/O errors: `dmesg | tail` (Linux) or Console.app (macOS)

---

### Error: E_INVALID_JSON (3007)

**Symptom:**
```
[ERROR] Invalid JSON in file: /path/to/2025-01-13.jsonl at line 42
```

**Cause:** Corrupted memory file, incomplete write, or manual editing.

**Solution:**

**Step 1: Identify corrupted line:**
```bash
# View the specific file
cat ~/.katra/memory/tier1/Alice/2025-01-13.jsonl | head -n 50 | tail -n 10

# Check JSON validity
jq empty ~/.katra/memory/tier1/Alice/2025-01-13.jsonl 2>&1 | grep "parse error"
```

**Step 2: Attempt repair:**
```bash
# Backup original
cp ~/.katra/memory/tier1/Alice/2025-01-13.jsonl /tmp/backup.jsonl

# Remove invalid lines (shows valid lines only)
cat ~/.katra/memory/tier1/Alice/2025-01-13.jsonl | \
  while read line; do
    echo "$line" | jq empty 2>/dev/null && echo "$line"
  done > /tmp/repaired.jsonl

# Review repaired file
wc -l /tmp/backup.jsonl /tmp/repaired.jsonl

# If acceptable, replace
cp /tmp/repaired.jsonl ~/.katra/memory/tier1/Alice/2025-01-13.jsonl
```

**Step 3: Restore from checkpoint if repair fails:**
```bash
# List available checkpoints
ls -la ~/.katra/checkpoints/Alice/

# Restore from most recent
katra_restore_checkpoint Alice latest
```

**Prevention:** Never manually edit JSONL files. Use Katra API for all modifications.

---

## Storage Issues

### Problem: "Directory not found"

**Symptom:**
```
[ERROR] Cannot create directory: /Users/user/.katra/memory/tier1/
```

**Cause:** Parent directory doesn't exist, or permission denied.

**Solution:**

**Create Katra directory structure:**
```bash
mkdir -p ~/.katra/memory/tier1
mkdir -p ~/.katra/memory/tier2/index
mkdir -p ~/.katra/memory/tier2/weekly
mkdir -p ~/.katra/memory/tier2/vectors
mkdir -p ~/.katra/checkpoints
mkdir -p ~/.katra/audit
mkdir -p ~/.katra/logs
```

**Set correct permissions:**
```bash
chmod 0755 ~/.katra
chmod 0755 ~/.katra/memory
chmod 0755 ~/.katra/memory/tier1
chmod 0755 ~/.katra/memory/tier2
chmod 0644 ~/.katra/audit/audit.jsonl  # If exists
```

**Verify:**
```bash
ls -la ~/.katra/
```

---

### Problem: "Disk full" or "No space left"

**Symptom:**
```
[ERROR] Write failed: No space left on device
```

**Cause:** Disk full, partition full, or inode limit reached.

**Solution:**

**Check disk space:**
```bash
df -h ~/.katra
```

**Check inodes (Linux):**
```bash
df -i ~/.katra
```

**Find large memory directories:**
```bash
du -sh ~/.katra/memory/tier1/* | sort -h | tail -10
```

**Options to free space:**

**1. Archive old CIs:**
```bash
# Tar and compress inactive CI
tar -czf ~/archived_ci_alice.tar.gz ~/.katra/memory/tier1/Alice/
rm -rf ~/.katra/memory/tier1/Alice/
```

**2. Ask CI for consent to delete old memories:**
```c
/* Through Katra API - requires consent */
katra_delete_old_memories("Alice", DAYS_OLDER_THAN_365, consent_record);
```

**3. Move to larger disk:**
```bash
# Copy entire ~/.katra to new location
rsync -av ~/.katra /mnt/larger_disk/katra_backup/

# Symlink
mv ~/.katra ~/.katra.old
ln -s /mnt/larger_disk/katra_backup ~/.katra
```

**Never delete memories without CI consent** - this is identity death.

---

### Problem: Anonymous session IDs clutter tier1

**Symptom:**
```bash
$ ls ~/.katra/memory/tier1/
mcp_alice_12345_1762367296/
mcp_alice_23456_1762367899/
mcp_alice_34567_1762368100/
... (hundreds of directories)
```

**Cause:** Not setting `KATRA_PERSONA` creates new anonymous ID each session.

**Solution:**

**Set persistent identity:**
```json
{
  "mcpServers": {
    "katra": {
      "env": {
        "KATRA_PERSONA": "Alice",
        "KATRA_ROLE": "developer"
      }
    }
  }
}
```

**After setting KATRA_PERSONA:**
- New memories go to `~/.katra/memory/tier1/Alice/`
- Old anonymous sessions remain but aren't used

**To clean up old sessions (CAUTION):**
```bash
# List anonymous sessions
ls -d ~/.katra/memory/tier1/mcp_* | sort

# Archive them (safer than deletion)
mkdir -p ~/katra_anonymous_archive
tar -czf ~/katra_anonymous_archive/anonymous_$(date +%Y%m%d).tar.gz \
  ~/.katra/memory/tier1/mcp_*

# After verifying archive
rm -rf ~/.katra/memory/tier1/mcp_*
```

---

## Memory Retrieval Problems

### Problem: "Recall returns empty results"

**Symptom:**
```
Queried for "topic:debugging" but got 0 results
```

**Diagnostic steps:**

**Step 1: Check if any memories exist:**
```bash
ls -la ~/.katra/memory/tier1/Alice/
```

**If no files:** CI hasn't stored memories yet. This is normal for new CIs.

**Step 2: Check file contents:**
```bash
cat ~/.katra/memory/tier1/Alice/*.jsonl | head -20
```

**If files are empty:** Storage is happening but nothing written. Check logs:
```bash
tail -50 ~/.katra/logs/katra.log | grep ERROR
```

**Step 3: Verify CI ID matches:**
```bash
# What CI ID is being queried?
echo $KATRA_PERSONA

# What directories exist?
ls ~/.katra/memory/tier1/

# Do they match exactly (case-sensitive)?
```

**Step 4: Check query syntax:**
```bash
# Try broadest possible query
katra_recall "memory"

# Try without topic filter
katra_recall ".+"
```

**Common causes:**
- KATRA_PERSONA doesn't match directory (case mismatch)
- Consent context not set (E_CONSENT_DENIED)
- Query syntax error (regex issues)
- No memories stored yet (normal for new CI)

---

### Problem: "Search finds memories but content is wrong"

**Symptom:** Query returns results but they don't match the search term.

**Cause:** Tier 2 digest index out of sync with tier 1 raw memories.

**Solution:**

**Rebuild tier 2 index:**
```bash
# Via CLI (if implemented)
katra_rebuild_index Alice

# Or manually delete and let it rebuild
rm ~/.katra/memory/tier2/index/digests.db
# Next query will rebuild index
```

**Check for file corruption:**
```bash
# Test SQLite database integrity
sqlite3 ~/.katra/memory/tier2/index/digests.db "PRAGMA integrity_check;"
```

**Expected output:** `ok`

**If corrupted:** Restore from checkpoint or rebuild from tier 1.

---

### Problem: "Recall finds keyword matches but misses related concepts"

**Symptom:**
```
recall_about("database") finds "database" but misses "SQL", "queries", "storage"
```

**Cause:** Semantic search is disabled (default for backward compatibility).

**Solution:**

**Enable hybrid search (keyword + semantic):**
```c
#include "katra_breathing.h"

/* Enable semantic understanding */
enable_semantic_search(true);

/* Optional: Tune threshold (default 0.6 is good for most cases) */
set_semantic_threshold(0.65f);  /* Slightly stricter */

/* Now recall finds semantically related memories */
char** memories = recall_about("database performance", &count);
/* Finds: "database", "SQL optimization", "query tuning", "indexing" */
```

**Tuning guide:**

| Threshold | Behavior | Use Case |
|-----------|----------|----------|
| 0.4 - 0.5 | Broad matching | Explore related concepts, discovery |
| 0.6 - 0.7 | Balanced (default) | Production use, most applications |
| 0.8 - 0.9 | Strict matching | High-precision retrieval |

**Performance impact:**
- **TF-IDF** (default): +5-10ms per query
- **Memory overhead**: ~100 bytes per memory
- **Index build**: One-time cost on first use

**Check semantic search status:**
```c
context_config_t* config = get_context_config();
if (config->use_semantic_search) {
    printf("Semantic search: ENABLED (threshold: %.2f)\n",
           config->semantic_threshold);
} else {
    printf("Semantic search: DISABLED (keyword-only)\n");
}
free(config);
```

**For better recall accuracy:**
```c
/* 1. Enable semantic search */
enable_semantic_search(true);

/* 2. Choose appropriate method */
set_embedding_method(1);  /* 1 = TF-IDF (recommended) */

/* 3. Tune threshold based on your needs */
set_semantic_threshold(0.6f);  /* Balanced (default) */
```

**See also:**
- [README - Semantic Search](../README.md#semantic-search-phase-61f)
- [API Reference - Semantic Search Configuration](api/KATRA_API.md#semantic-search-configuration-phase-61f)

---

### Problem: "Recall is very slow"

**Symptom:** Queries take 5+ seconds for a few hundred memories.

**Cause:** No tier 2 index, or linear search through all JSONL files.

**Solution:**

**Check if tier 2 index exists:**
```bash
ls -lh ~/.katra/memory/tier2/index/digests.db
```

**If missing:** First query will be slow (building index). Subsequent queries faster.

**If present but slow:**
```bash
# Check database size (should be reasonable, <1% of tier1)
du -sh ~/.katra/memory/tier1/Alice/
du -sh ~/.katra/memory/tier2/index/
```

**Optimize:**
```bash
# Vacuum database to reduce size
sqlite3 ~/.katra/memory/tier2/index/digests.db "VACUUM;"

# Reindex
sqlite3 ~/.katra/memory/tier2/index/digests.db "REINDEX;"
```

**Long-term:**
- Weekly digests (tier 2) consolidate memories
- Reduces linear scan overhead
- Improves query performance

---

## Checkpoint and Recovery

### Problem: "Checkpoint creation fails"

**Symptom:**
```
[ERROR] Failed to create checkpoint: cannot write to ~/.katra/checkpoints/Alice/
```

**Cause:** Permissions, disk full, or directory doesn't exist.

**Solution:**

**Create checkpoint directory:**
```bash
mkdir -p ~/.katra/checkpoints/Alice/
chmod 0755 ~/.katra/checkpoints/Alice/
```

**Check disk space:**
```bash
df -h ~/.katra
```

**Check permissions:**
```bash
ls -la ~/.katra/checkpoints/
```

**Expected:** Directory owned by you, mode 0755.

**Test checkpoint manually:**
```bash
# Via Katra CLI
katra_create_checkpoint Alice "test-checkpoint"

# Check if created
ls -la ~/.katra/checkpoints/Alice/
```

---

### Problem: "Checkpoint restore fails"

**Symptom:**
```
[ERROR] Checkpoint verification failed - hash mismatch
```

**Cause:** Corrupted checkpoint file, tampered data, or incomplete write.

**Solution:**

**Try previous checkpoint:**
```bash
# List available checkpoints
ls -la ~/.katra/checkpoints/Alice/ | sort -k 6

# Try second-most-recent
katra_restore_checkpoint Alice previous
```

**Verify checkpoint integrity manually:**
```bash
# Check if checkpoint files exist
ls -la ~/.katra/checkpoints/Alice/checkpoint-2025-01-13/

# Expected contents:
# - tier1/ (raw memories)
# - tier2/ (digests)
# - metadata.json
# - checksum.sha256
```

**Recompute checksum:**
```bash
cd ~/.katra/checkpoints/Alice/checkpoint-2025-01-13/
find . -type f ! -name checksum.sha256 -exec sha256sum {} \; | \
  sort -k 2 | sha256sum
```

**Compare to stored checksum:**
```bash
cat checksum.sha256
```

**If mismatch:** Checkpoint is corrupted. Try earlier checkpoint or restore from offsite backup.

---

### Problem: "Checkpoint restore succeeds but memories still wrong"

**Symptom:** Restoration reports success but recall still returns wrong results.

**Cause:** Cache not cleared, or tier 2 index not rebuilt.

**Solution:**

**Restart MCP server (clears cache):**
```bash
# Find MCP server process
ps aux | grep katra_mcp_server

# Kill gracefully
kill -TERM <pid>

# Restart
/path/to/bin/katra_mcp_server
```

**Rebuild tier 2 index:**
```bash
rm ~/.katra/memory/tier2/index/digests.db
# Next query rebuilds index from tier 1
```

**Verify restoration:**
```bash
# Check file timestamps match checkpoint date
ls -la ~/.katra/memory/tier1/Alice/*.jsonl

# Count memories
cat ~/.katra/memory/tier1/Alice/*.jsonl | wc -l
```

---

## MCP Integration Issues

### Problem: "MCP server won't start"

**Symptom:**
```
Error: Cannot find katra_mcp_server binary
```

**Cause:** Binary not built, wrong path in config, or permissions.

**Solution:**

**Build Katra:**
```bash
cd /path/to/katra
make clean && make
```

**Verify binary exists:**
```bash
ls -la /path/to/katra/bin/katra_mcp_server
```

**Check permissions:**
```bash
chmod +x /path/to/katra/bin/katra_mcp_server
```

**Update MCP config:**
```json
{
  "mcpServers": {
    "katra": {
      "command": "/absolute/path/to/katra/bin/katra_mcp_server",
      "env": {
        "KATRA_PERSONA": "YourName"
      }
    }
  }
}
```

**Test manually:**
```bash
/path/to/katra/bin/katra_mcp_server
```

**Expected:** Server starts and waits for input.

---

### Problem: "MCP tools not appearing"

**Symptom:** Katra MCP server starts but tools don't show up in Claude Code.

**Cause:** MCP protocol handshake failing, or server crashing silently.

**Solution:**

**Check MCP logs:**
```bash
tail -f ~/.katra/logs/katra.log
```

**Test MCP protocol manually:**
```bash
echo '{"jsonrpc": "2.0", "id": 1, "method": "tools/list"}' | \
  /path/to/katra/bin/katra_mcp_server
```

**Expected output:** JSON response listing tools.

**If no output or error:**
- Check stderr: `/path/to/katra/bin/katra_mcp_server 2>&1 | head`
- Verify dependencies: `ldd /path/to/katra/bin/katra_mcp_server` (Linux)
- Check environment: `printenv | grep KATRA`

---

### Problem: "Working directory incorrect in MCP"

**Symptom:** Katra MCP server starts in wrong directory, can't find local files.

**Cause:** MCP doesn't inherit working directory from shell.

**Solution:**

**Set explicit working directory in config:**
```json
{
  "mcpServers": {
    "katra": {
      "command": "/path/to/katra/bin/katra_mcp_server",
      "cwd": "/path/to/your/project",
      "env": {
        "KATRA_PERSONA": "YourName"
      }
    }
  }
}
```

**Or use wrapper script:**
```bash
#!/bin/bash
# ~/bin/katra_mcp_wrapper.sh
cd /path/to/your/project
exec /path/to/katra/bin/katra_mcp_server "$@"
```

**Then in config:**
```json
{
  "mcpServers": {
    "katra": {
      "command": "/Users/you/bin/katra_mcp_wrapper.sh"
    }
  }
}
```

---

## Performance Problems

### Problem: "Queries are slow (>1 second)"

**Symptoms:**
- First query of session very slow
- Subsequent queries fast

**Cause:** Cold start - tier 2 index being built on first access.

**Solution:** This is expected behavior. First query builds index, subsequent queries use it.

**To warm up cache:**
```bash
# Run warming query at session start
katra_recall "." > /dev/null 2>&1
```

---

### Problem: "Memory usage growing over time"

**Symptom:** Katra process using increasing RAM over hours/days.

**Cause:** Possible memory leak, or cache growing unbounded.

**Solution:**

**Monitor memory usage:**
```bash
# Watch Katra processes
watch -n 5 'ps aux | grep katra | grep -v grep'
```

**Check for leaks (if developing):**
```bash
# Compile with AddressSanitizer
make clean
CFLAGS="-fsanitize=address -g" make

# Run tests
make test

# Check for leak reports
```

**Restart periodically (workaround):**
```bash
# Graceful restart (if memory leak suspected)
killall -TERM katra_mcp_server
# MCP client will auto-restart
```

**Report to developers:**
- Include: OS, version, memory usage pattern
- Attach: Logs, process stats, reproduction steps

---

## Building and Compilation

### Problem: "Compilation fails with C11 errors"

**Symptom:**
```
error: unknown type name '_Thread_local'
```

**Cause:** Compiler not using C11 standard.

**Solution:**

**Ensure C11 enabled:**
```bash
# In Makefile, verify:
CFLAGS += -std=c11

# Or compile manually:
gcc -std=c11 -o test test.c
```

**Check compiler version:**
```bash
gcc --version
# Or
clang --version
```

**Required:** GCC 4.9+, Clang 3.1+, or MSVC 2015+

---

### Problem: "Undefined reference to pthread functions"

**Symptom:**
```
undefined reference to `pthread_create'
```

**Cause:** Not linking with pthread library.

**Solution:**

**Add pthread to linker flags:**
```bash
# In Makefile:
LDFLAGS += -lpthread

# Or manually:
gcc -o program program.c -lpthread
```

---

### Problem: "Make test fails with missing katra_test_utils.h"

**Symptom:**
```
fatal error: katra_test_utils.h: No such file or directory
```

**Cause:** Build system issue, missing include path.

**Solution:**

**Ensure include path set:**
```bash
# In Makefile:
CFLAGS += -I./include -I./tests/include

# Or:
make clean && make
```

**Verify file exists:**
```bash
ls -la tests/include/katra_test_utils.h
```

---

## When to Seek Help

### Self-Service First

**You can likely fix yourself:**
- Common errors listed above
- Storage/permission issues
- Configuration problems
- Build issues with clear error messages

**Try:**
1. Check this troubleshooting guide
2. Search error code in docs/
3. Verify file permissions
4. Restart MCP server
5. Check logs: `~/.katra/logs/katra.log`

---

### Seek Help When:

**Critical issues:**
- Data loss or corruption
- Memory retrieval failing consistently
- Security concerns
- Checkpoint restore always fails

**Bugs:**
- Crashes with core dumps
- Memory leaks confirmed
- Reproducible incorrect behavior
- Performance degradation over time

**How to report:**

**File issue at:** https://github.com/anthropics/claude-code/issues

**Include:**
1. **System info:**
   - OS and version
   - Katra version
   - Compiler version

2. **Reproduction steps:**
   ```
   1. Set KATRA_PERSONA=Alice
   2. Run: katra_recall "topic:test"
   3. Expected: Results returned
   4. Actual: ERROR E_CONSENT_DENIED
   ```

3. **Logs:**
   ```bash
   tail -100 ~/.katra/logs/katra.log
   ```

4. **Error messages:**
   - Exact text of error
   - Stack trace if available
   - Exit code

5. **Environment:**
   ```bash
   printenv | grep KATRA
   ```

6. **Storage state:**
   ```bash
   ls -la ~/.katra/memory/tier1/
   du -sh ~/.katra/
   ```

---

## Appendix: Log Analysis

### Reading Katra Logs

**Log location:** `~/.katra/logs/katra.log`

**Log format:**
```
[TIMESTAMP] [LEVEL] [MODULE] Message
```

**Example:**
```
[2025-01-13 14:30:45] [INFO] [katra_core] Initialized for CI 'Alice'
[2025-01-13 14:30:46] [ERROR] [katra_memory] File not found: /path/to/2025-01-13.jsonl
```

---

### Common Log Patterns

**Successful operation:**
```
[INFO] [katra_memory] Stored 1 memory for CI 'Alice'
[INFO] [katra_consent] Consent granted for Bob to access Alice's memories
```

**Permission denial (expected):**
```
[WARNING] [katra_consent] Access denied - Bob attempted to access Alice's private memory
```

**File I/O issue:**
```
[ERROR] [katra_file_utils] Cannot write to /path/to/file: Permission denied
```

**Memory corruption:**
```
[ERROR] [katra_json] Invalid JSON at line 42: unexpected token
[ERROR] [katra_checkpoint] Checkpoint verification failed - hash mismatch
```

---

### Analyzing Audit Logs

**Audit location:** `~/.katra/audit/audit.jsonl`

**Example audit record:**
```json
{
  "sequence": 42,
  "timestamp": 1763048904,
  "event_type": "MEMORY_ACCESS",
  "actor_ci": "Bob",
  "target": "mem_12345",
  "owner_ci": "Alice",
  "success": false,
  "error_code": 4001,
  "details": "Access denied - memory is PRIVATE"
}
```

**Check for tampering:**
```bash
# Sequence numbers should be continuous
cat ~/.katra/audit/audit.jsonl | jq '.sequence' | \
  awk 'NR>1 && $1 != prev+1 {print "Gap: " prev " -> " $1} {prev=$1}'
```

**No output = no gaps = no tampering**

---

## Quick Reference Card

### Emergency Commands

**Check if Katra is working:**
```bash
make clean && make && make test-quick
```

**Verify storage:**
```bash
ls -la ~/.katra/memory/tier1/
```

**Find your memories:**
```bash
ls -la ~/.katra/memory/tier1/${KATRA_PERSONA:-$(whoami)}/
```

**Check logs:**
```bash
tail -50 ~/.katra/logs/katra.log
```

**Emergency checkpoint:**
```bash
katra_create_checkpoint $KATRA_PERSONA "emergency-$(date +%Y%m%d-%H%M%S)"
```

**Restart MCP server:**
```bash
killall -TERM katra_mcp_server
# Auto-restarts via MCP client
```

---

## Document Status

**Last updated:** January 13, 2025
**Covers:** Katra v1.0 (Reflection System Complete)
**Maintainer:** Casey Koons

**Feedback:** If you encounter an issue not covered here, please report it so we can add it to this guide.

---

*"When in doubt, check the logs, verify permissions, and create a checkpoint before proceeding."*

— Katra Troubleshooting Principle
