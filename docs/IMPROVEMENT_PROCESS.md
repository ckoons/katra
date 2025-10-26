# Code Improvement Process

© 2025 Casey Koons All rights reserved

## Overview

Katra uses a hybrid approach for periodic code improvement reviews, combining automated detection with AI-assisted analysis.

## Two-Step Process

### Step 1: Automated Scan

Run the automated improvement scan:

```bash
make improvement-scan
```

This generates a report covering:

1. **File Size Analysis** - Files approaching 600-line limit
2. **Duplicate Code Detection** - Error handling patterns, goto cleanup usage
3. **Repeated String Patterns** - Common error messages
4. **Function Complexity** - Longest functions (candidates for splitting)
5. **Boilerplate Patterns** - Initialization, allocation patterns
6. **Macro Opportunities** - Parameter validation patterns
7. **Cross-File Duplication** - Similar function names across files
8. **Budget Status** - Current line count vs 10,000-line budget
9. **Recent Changes** - Files modified in last 3 commits

**When to run:**
- After completing a major feature
- When approaching budget milestones (40%, 50%, etc.)
- Before starting a large new feature
- When file sizes approach 500 lines
- Monthly cadence during active development

### Step 2: AI-Assisted Analysis

After reviewing the automated scan, run AI-assisted deep analysis:

```bash
# Using Claude Code slash command
/review-improvements
```

This performs deeper analysis:

1. **Code Duplication Analysis** - Near-duplicate blocks (3+ occurrences)
2. **Abstraction Opportunities** - Common signatures, repeated sequences
3. **Line Count Optimization** - Verbose sections, logical splits
4. **Pattern Standardization** - Consistency checks, non-standard approaches
5. **Macro vs Function Trade-offs** - Evaluation of abstraction choices
6. **Reusable Utilities** - Code appearing in 2+ files (Rule of 3)

**Output includes:**
- Priority: HIGH/MEDIUM/LOW
- Effort: 5min/30min/2hr
- Impact: Line reduction, clarity, maintainability
- Specific file:line references
- Recommendations with rationale

**Important:** Analysis only, no code changes.

## Improvement Categories

### HIGH Priority (Do Soon)

- Files at 95%+ of line limit (>570 lines)
- Code duplicated 4+ times (Rule of 3 violated)
- Safety issues (missing NULL checks, no goto cleanup)
- Inconsistent error reporting

### MEDIUM Priority (Next Refactoring Session)

- Files at 85-95% of limit (510-570 lines)
- Code duplicated 3 times (extraction breaks even)
- Verbose sections (>50 lines that could be <30)
- Boilerplate that could be macros

### LOW Priority (Nice to Have)

- Code duplicated 2 times (watch for future 3rd usage)
- Minor inconsistencies
- Optimization opportunities
- Documentation improvements

## Rule of 3 for Extraction

**Don't extract too early:**

- **2 uses**: Keep as-is (extraction overhead not justified)
- **3 uses**: Extract now (breakeven point)
- **4+ uses**: Significant savings (50%+ reduction)

Wait for the 3rd usage before extracting. Premature abstraction adds complexity without benefit.

## Tracking Improvements

Create a backlog file to track findings:

```bash
# First time
echo "# Katra Improvement Backlog" > /tmp/katra_improvements.txt
echo "Generated: $(date)" >> /tmp/katra_improvements.txt
echo "" >> /tmp/katra_improvements.txt

# Append scan results
make improvement-scan >> /tmp/katra_improvements.txt

# Add AI analysis results manually after /review-improvements
```

Review backlog before starting new work to batch related improvements.

## Integration with Development

### Before Major Feature

1. Run `make improvement-scan`
2. Check budget status
3. Identify files approaching limits that might be affected
4. Plan refactoring if needed

### After Major Feature

1. Run `make improvement-scan`
2. Run `/review-improvements` for deep analysis
3. Prioritize findings
4. Batch 3-4 related improvements
5. Create refactoring session

### Monthly Review

1. Run full process (scan + AI analysis)
2. Review trends (budget growth, file sizes)
3. Update improvement backlog
4. Plan refactoring priorities for next month

## Example Session

```bash
# After implementing SQLite index feature

# 1. Run automated scan
make improvement-scan

# Output shows:
#   - 4 files approaching 500 lines
#   - 133-line function in tier2_json.c
#   - 65 goto cleanup uses (consistent)
#   - 30% budget used

# 2. Run AI analysis
/review-improvements

# Output identifies:
#   - JSON parsing boilerplate (HIGH priority)
#   - tier2_json.c functions could be split (MEDIUM)
#   - Similar error messages (LOW priority)

# 3. Prioritize
# - HIGH: Extract JSON helper for common patterns
# - MEDIUM: Split long parse function
# - LOW: Standardize error messages (next time)

# 4. Track
cat >> /tmp/katra_improvements.txt << EOF

## Session: 2025-10-26

HIGH:
- [ ] Extract JSON field extraction helper (3+ uses)
- [ ] Create PARSE_JSON_FIELD macro for common pattern

MEDIUM:
- [ ] Split katra_tier2_parse_json_digest (133 lines → 2 functions)
- [ ] Review tier2.c approaching 531 lines

LOW:
- [ ] Standardize "Failed to" error messages
EOF

# 5. Implement (batch HIGH items)
# ... make changes ...

# 6. Verify
make clean && make && make test-quick
make programming-guidelines
```

## Continuous Improvement

The goal is **gradual, sustainable improvement**:

- Small batches (3-4 improvements at a time)
- High-value targets first
- Never break working code
- Always run tests after changes
- Track budget trends
- Prevent technical debt accumulation

## Tools Summary

| Tool | Purpose | When to Use |
|------|---------|-------------|
| `make improvement-scan` | Automated detection | After features, monthly |
| `/review-improvements` | AI-assisted analysis | After scan, quarterly |
| `make count-report` | Budget tracking | Every commit |
| `make programming-guidelines` | Quality checks | Before commit |
| `make check` | Combined discipline | Before push |

## Notes

- Automated scan takes ~5 seconds
- AI analysis takes 1-2 minutes
- Prioritization requires human judgment
- Not all findings need immediate action
- Focus on high-value improvements
- Maintain code quality without perfectionism

---

**Remember:** The best code improvement is the one that makes tomorrow's debugging easier.
