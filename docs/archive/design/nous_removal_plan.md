# Nous Tools Removal Plan

© 2025 Casey Koons. All rights reserved.

**Created:** 2025-11-16
**Status:** Approved for Removal
**Target:** Move to Argo Software Engineering MCP

---

## Executive Summary

The Nous tools (katra_placement, katra_impact, katra_user_domain) are being removed from Katra and moved to Argo's Software Engineering MCP server. These tools provide code intelligence and architectural guidance, which belong with workflow coordination (Argo) rather than memory/identity (Katra).

**Reason:** Katra should focus solely on persistent memory and identity continuity. Software engineering tools dilute this clear purpose.

---

## What's Being Removed

### MCP Tools (3 tools)

1. **katra_placement** - "Where should this code go?"
   - Architecture guidance
   - File organization recommendations
   - Pattern-based placement

2. **katra_impact** - "What breaks if I change this?"
   - Dependency analysis
   - Change impact prediction
   - Risk assessment

3. **katra_user_domain** - "Who uses this feature?"
   - User/feature domain modeling
   - Usage pattern analysis
   - Persona identification

### Implementation Files

**MCP Layer:**
```
src/mcp/mcp_nous.c                    113 lines
```

**Nous Engine:**
```
src/nous/katra_nous_compose.c         varies
src/nous/katra_nous_patterns.c        varies
src/nous/katra_nous_impact.c          varies
src/nous/katra_nous_reasoning.c       varies
src/nous/katra_nous_crossproject.c    varies
src/nous/katra_nous_common.c          varies
```

**Headers:**
```
include/katra_nous.h                  658 lines
```

**Documentation:**
```
docs/guide/MCP_TOOLS.md               (nous section)
```

**Estimated total:** ~2,000 lines of code

---

## Files to Delete

### Source Files

```bash
# MCP implementation
rm src/mcp/mcp_nous.c

# Nous engine (entire directory)
rm -rf src/nous/

# Header
rm include/katra_nous.h
```

### Documentation Updates

**Update these files:**
```
docs/guide/MCP_TOOLS.md       # Remove nous tools section
docs/guide/MCP_SERVER.md      # Remove nous tools from resources list
README.md                     # Remove nous tools from features
```

**Add migration notice:**
```
docs/MIGRATION.md             # New file explaining removal and migration path
```

---

## Migration Path

### Phase 1: Deprecation Notice (Immediate)

Add deprecation warnings to MCP tool responses:

```c
/* src/mcp/mcp_nous.c */
json_t* mcp_tool_placement(json_t* args, json_t* id) {
    LOG_WARN("katra_placement is deprecated and will be removed.");
    LOG_WARN("Use argo-se-mcp for code intelligence tools.");

    /* Continue functioning */
    const char* query_text = json_string_value(json_object_get(args, "query"));
    return execute_nous_query(query_text, QUERY_TYPE_PLACEMENT, "placement");
}
```

Update documentation:

```markdown
## Deprecated Tools

⚠️ **DEPRECATED:** The following tools are deprecated and will be removed:

- `katra_placement()` → Use `argo-se-mcp` instead
- `katra_impact()` → Use `argo-se-mcp` instead
- `katra_user_domain()` → Use `argo-se-mcp` instead

These tools are being moved to Argo's Software Engineering MCP server where they
better fit with workflow coordination and code analysis capabilities.

See: https://github.com/caseykoons/argo/docs/plans/software-engineering-mcp.md
```

### Phase 2: Removal (After Argo SE-MCP Ready)

**Timing:** Wait until Argo Software Engineering MCP is implemented and tested

**Pre-removal checklist:**
- [ ] Argo SE-MCP implements all three tools
- [ ] Argo SE-MCP has feature parity with Katra nous
- [ ] Documentation in Argo is complete
- [ ] Migration guide is written
- [ ] Deprecation warnings have been in place for at least 1 month

**Removal steps:**

1. Delete source files:
   ```bash
   git rm src/mcp/mcp_nous.c
   git rm -r src/nous/
   git rm include/katra_nous.h
   ```

2. Remove from Makefile:
   ```makefile
   # Remove from SOURCES
   NOUS_SOURCES = \
       src/nous/katra_nous_compose.c \
       src/nous/katra_nous_patterns.c \
       # ... etc

   # Remove from linking
   libkatra_nous.a: $(NOUS_OBJECTS)
   ```

3. Remove from MCP server:
   ```c
   /* src/mcp/mcp_protocol.c */
   /* Delete tool registrations */
   // mcp_register_tool(server, "katra_placement", mcp_tool_placement);
   // mcp_register_tool(server, "katra_impact", mcp_tool_impact);
   // mcp_register_tool(server, "katra_user_domain", mcp_tool_user_domain);
   ```

4. Update documentation:
   - Remove nous tools from MCP_TOOLS.md
   - Remove from features list
   - Add to MIGRATION.md

5. Test that everything still builds and passes tests

6. Commit with clear message:
   ```
   Remove nous tools - moved to argo-se-mcp

   The katra_placement, katra_impact, and katra_user_domain tools have been
   removed from Katra and moved to Argo's Software Engineering MCP server.

   Katra now focuses solely on persistent memory and identity continuity.
   Code intelligence tools belong with workflow coordination in Argo.

   See: argo/docs/plans/software-engineering-mcp.md

   Removed:
   - src/mcp/mcp_nous.c (113 lines)
   - src/nous/ (entire directory, ~1500 lines)
   - include/katra_nous.h (658 lines)

   Total reduction: ~2000 lines
   ```

### Phase 3: Communication (Ongoing)

**Update all documentation:**

1. **README.md:**
   ```markdown
   ## Features

   Katra provides:
   - Persistent memory (tier1/tier2/tier3)
   - Identity continuity (personas, checkpoints)
   - Session continuity (breathing, snapshots)
   - CI communication (meeting room)
   - Memory consolidation (sunset/sunrise)

   **Note:** Code intelligence tools (placement, impact analysis) have moved to
   [Argo SE-MCP](https://github.com/caseykoons/argo).
   ```

2. **docs/MIGRATION.md** (new file):
   ```markdown
   # Migration Guide: Nous Tools Removal

   ## Overview

   As of [DATE], the nous tools have been removed from Katra and moved to Argo's
   Software Engineering MCP server.

   ## Removed Tools

   - `katra_placement()` → Use `argo-se-mcp` `argo_placement()`
   - `katra_impact()` → Use `argo-se-mcp` `argo_impact()`
   - `katra_user_domain()` → Use `argo-se-mcp` `argo_user_domain()`

   ## Why Removed?

   Katra's purpose is persistent memory and identity continuity. Code intelligence
   belongs with workflow coordination in Argo.

   **Katra = Memory/Identity**
   **Argo = Workflow/Code Intelligence**

   ## How to Migrate

   1. Install Argo: https://github.com/caseykoons/argo
   2. Start argo-daemon: `argo-daemon`
   3. Use arc CLI: `arc code placement "Where should this go?"`

   Or use the MCP server directly:

   ```bash
   # Configure Claude Code to use argo-se-mcp
   # Add to .claude/mcp.json:
   {
     "mcpServers": {
       "argo-se": {
         "command": "/path/to/argo-se-mcp"
       }
     }
   }
   ```

   ## Benefits of Migration

   Argo SE-MCP provides enhanced capabilities:
   - Git integration (analyze commit history)
   - Workflow-aware recommendations
   - Multi-CI coordination
   - Real codebase analysis
   - Dependency graph visualization

   See: argo/docs/plans/software-engineering-mcp.md
   ```

---

## Impact Analysis

### Line Count Reduction

**Before removal:**
- Total Katra lines: ~20,686 (current)

**After removal:**
- Removed: ~2,000 lines
- New total: ~18,686 lines
- Budget remaining: 11,314 lines (of 30,000)

### Dependency Cleanup

**Removed dependencies:**
- None (nous used existing katra infrastructure)

**Simplified architecture:**
- No more code analysis in memory layer
- Clear separation: memory vs code intelligence
- Reduced cognitive load for katra users

### API Changes

**Removed from MCP:**
```
katra_placement    (REMOVED)
katra_impact       (REMOVED)
katra_user_domain  (REMOVED)
```

**Remaining MCP tools:**
```
katra_remember     (memory storage)
katra_recall       (memory retrieval)
katra_recent       (chronological access)
katra_learn        (knowledge storage)
katra_decide       (decision recording)
katra_register     (identity)
katra_whoami       (identity)
katra_say          (CI communication)
katra_hear         (CI communication)
katra_who_is_here  (CI communication)
```

**Result:** All remaining tools are memory/identity related ✓

---

## Testing After Removal

### Verification Tests

1. **Build succeeds:**
   ```bash
   make clean && make
   ```

2. **All tests pass:**
   ```bash
   make test
   ```

3. **MCP server works:**
   ```bash
   echo '{"jsonrpc":"2.0","id":1,"method":"tools/list"}' | ./bin/katra_mcp_server
   # Should NOT list nous tools
   ```

4. **Memory tools still work:**
   ```bash
   echo '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"katra_remember","arguments":{"content":"test","context":"trivial"}}}' | ./bin/katra_mcp_server
   ```

5. **Line count budget:**
   ```bash
   make count-report
   # Should show ~18,686 lines
   ```

### Regression Prevention

**Add test to prevent re-introduction:**

```bash
# scripts/test_no_nous.sh
#!/bin/bash
# Verify nous tools are not present

if grep -r "katra_placement\|katra_impact\|katra_user_domain" src/ include/; then
    echo "ERROR: Nous tools found in codebase"
    echo "These should be in Argo SE-MCP, not Katra"
    exit 1
fi

echo "✓ No nous tools found (correct)"
exit 0
```

---

## Rollback Plan (If Needed)

If Argo SE-MCP is not ready in time:

1. **Keep nous tools functional** during transition period
2. **Maintain deprecation warnings** but don't remove code
3. **Set deadline** for Argo SE-MCP completion
4. **Only remove after** Argo SE-MCP has feature parity

**Criteria for removal:**
- Argo SE-MCP implements all three tools
- Argo SE-MCP has >= 90% test coverage
- At least 2 users have successfully migrated
- Documentation is complete
- Deprecation warnings have been visible for >= 1 month

---

## Communication Timeline

**Week 1:** Add deprecation warnings to code and docs
**Week 2-4:** Argo SE-MCP development
**Week 5:** Announce removal timeline
**Week 6-8:** Monitor for user issues
**Week 9:** Remove code if no blockers

---

## Success Criteria

1. **Code removed:** All nous files deleted
2. **Build succeeds:** make clean && make works
3. **Tests pass:** All remaining tests pass (100%)
4. **Documentation updated:** No references to removed tools
5. **Line count reduced:** ~2,000 lines removed from budget
6. **Clear purpose:** Katra is purely memory/identity
7. **Migration path:** Users can easily switch to Argo SE-MCP

---

## Owner

**Casey Koons** (casey@...)

**Questions/Issues:** Create GitHub issue or contact directly

---

## References

- Argo SE-MCP Design: `/Users/cskoons/projects/github/argo/docs/plans/software-engineering-mcp.md`
- Katra Nous Implementation: `/Users/cskoons/projects/github/katra/src/nous/`
- Katra Architecture: `/Users/cskoons/projects/github/katra/docs/guide/ARCHITECTURE.md`

---

**Next Steps:**

1. ✅ Document current implementation (this file)
2. ✅ Design Argo SE-MCP (see argo/docs/plans/software-engineering-mcp.md)
3. ⏳ Add deprecation warnings to Katra
4. ⏳ Implement Argo SE-MCP
5. ⏳ Remove nous tools from Katra
6. ⏳ Update all documentation

