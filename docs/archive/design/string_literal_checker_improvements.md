# String Literal Checker Improvements

## Current Issues

### 1. Comment Scanning Problems

**Current behavior:**
```bash
grep -v ":[[:space:]]*//" | grep -v ":[[:space:]]*\*" | grep -v ":[[:space:]]/\*"
```

This only filters lines that START with comment markers. It misses:

```c
/* Access-count weighting: frequent access extends "warm" period */
int foo = bar; /* This is "inline" */
code; // Comment with "string"
```

**Why it's failing:**
- Line 103 in `katra_tier1_archive.c`: `/* Access-count weighting... */`
  - This IS a comment-only line, but the regex doesn't match it
  - The line starts with whitespace, then `/*`
  - Pattern `:[[:space:]]*\/\*` requires colon immediately before optional space
  - But grep output is `filename:linenum:    /* comment */`
  - The whitespace is AFTER the colon+linenum, not before

### 2. No SQL Pattern Recognition

**Currently flagged as unapproved:**
```c
"CREATE TABLE IF NOT EXISTS teams (" \
"  team_name TEXT PRIMARY KEY," \
"  owner_ci_id TEXT NOT NULL," \
```

**These are legitimate SQL DDL constants** that belong inline, not in headers.

SQL patterns to auto-approve:
- `CREATE TABLE`, `CREATE INDEX`, `CREATE VIEW`
- `INSERT INTO`, `INSERT OR REPLACE`
- `SELECT ... FROM`, `UPDATE`, `DELETE FROM`
- `ALTER TABLE`, `DROP TABLE`
- `PRAGMA`

### 3. Multi-line String Literals

**Currently flagged:**
```c
static const char* SQL_CREATE_TEAMS =
    "CREATE TABLE IF NOT EXISTS teams ("  \
    "  team_name TEXT PRIMARY KEY,"       \
    "  owner_ci_id TEXT NOT NULL"         \
    ")";
```

**Problem:** Each continuation line is counted separately
**Solution:** Detect `static const char* ... =` pattern (already has this at line 618, 634, 702, 717, 753, 767)

## Proposed Fixes

### Fix 1: Improved Comment Detection

**Replace:**
```bash
grep -v ":[[:space:]]*//" | grep -v ":[[:space:]]*\*" | grep -v ":[[:space:]]/\*"
```

**With:**
```bash
awk '
  # Remove C++ style comments
  { gsub(/\/\/.*$/, "") }

  # Remove C style comments (simple, doesn match nested)
  { gsub(/\/\*.*\*\//, "") }

  # Skip lines that are now empty or whitespace-only
  /^[[:space:]]*$/ { next }

  # Print non-comment content
  { print }
'
```

**Better approach - Pre-process to strip ALL comments:**
```bash
# Create a preprocessing step that strips comments before string detection
preprocess_strip_comments() {
  awk '
    BEGIN { in_comment = 0 }

    # Handle block comments
    {
      line = $0
      result = ""
      i = 1
      while (i <= length(line)) {
        if (in_comment) {
          # Look for end of comment
          if (substr(line, i, 2) == "*/") {
            in_comment = 0
            i += 2
            continue
          }
        } else {
          # Look for start of comment
          if (substr(line, i, 2) == "/*") {
            in_comment = 1
            i += 2
            continue
          }
          # Look for line comment
          if (substr(line, i, 2) == "//") {
            break  # Rest of line is comment
          }
          # Regular character
          result = result substr(line, i, 1)
        }
        i++
      }

      # Print result if non-empty
      if (length(result) > 0 && result !~ /^[[:space:]]*$/) {
        print FILENAME":"NR":"result
      }
    }
  ' FILENAME="$file" "$file"
}
```

### Fix 2: SQL Pattern Recognition

**Add to auto-approval patterns (line ~618, ~702, ~752):**

```bash
grep -vE "CREATE (TABLE|INDEX|VIEW)|INSERT (INTO|OR REPLACE)|SELECT .* FROM|UPDATE .*SET|DELETE FROM|ALTER TABLE|DROP TABLE|PRAGMA"
```

**More specific - only approve if it's a SQL DDL assignment:**

```bash
grep -vE "SQL_[A-Z_]*.*=.*\"(CREATE|INSERT|SELECT|UPDATE|DELETE|ALTER|DROP|PRAGMA)"
```

This ensures we only approve SQL strings assigned to `SQL_*` constants.

### Fix 3: Better Multi-line Detection

**Current pattern (line 634, 717, 767):**
```bash
const char\*[[:space:]]*[a-zA-Z_].*=[[:space:]]*\"
```

This already catches `const char* SQL_FOO = "..."`

**But it doesn't catch continuation lines:**
```c
static const char* SQL_FOO =
    "CREATE TABLE ..."  \    // ← This line
    "  column1 TEXT"    \    // ← And this line
    ")";                     // ← And this line
```

**Solution:** Detect line-continuation backslash:

```bash
grep -vE "\"[[:space:]]*\\\\$"
```

This filters any line ending with `" \`

## Implementation

### Option A: Patch Current Script (Minimal Changes)

Add these patterns to the existing grep chains:

```bash
# After line 618, 702, 752, add:
grep -vE "SQL_[A-Z_]*.*=.*\"(CREATE|INSERT|SELECT|UPDATE|DELETE|ALTER|DROP|PRAGMA)" | \
grep -vE "\"[[:space:]]*\\\\[[:space:]]*$" |
```

And improve comment filtering by adding a preprocessing pass at line 676:

```bash
# Instead of the current awk at line 677-681, use comment-stripping awk
```

### Option B: Rewrite with Better Architecture (Recommended)

Create a proper string literal analyzer:

```bash
analyze_string_literals() {
  local file="$1"

  # Step 1: Strip comments
  local content=$(strip_comments "$file")

  # Step 2: Extract string literals
  local strings=$(echo "$content" | extract_strings)

  # Step 3: Classify each string
  echo "$strings" | while read -r line; do
    classify_string "$line"
  done
}

strip_comments() {
  # Use proper C comment stripper (awk as shown above)
}

extract_strings() {
  # Extract all "..." literals with context
}

classify_string() {
  # Check against auto-approval patterns
  # Return: approved | unapproved | needs-review
}
```

## Auto-Approval Patterns (Complete List)

### Already Approved (Working Correctly)
✅ Format strings (containing `%`)
✅ Log messages (`LOG_*`, `printf`, `fprintf`, `snprintf`)
✅ Error contexts (`katra_report_error`)
✅ String comparison (`strcmp`, `strncmp`, `strstr`, etc.)
✅ Process execution (`execlp`, `execv`, etc.)
✅ File operations (`fopen`, `popen`, `freopen`)
✅ Environment access (`getenv`, `setenv`, `putenv`)
✅ Struct field init (`.field = "value"`)
✅ Array init (`= { "value" }`)
✅ Return statements (`return "value"`)
✅ Single-char literals (`== "x"`, `!= "\n"`)
✅ HTTP responses (`http_response_set_error`)
✅ API paths (`"/api/..."`)
✅ Const assignments (`const char* var = "..."`)
✅ File output (`fprintf(fp, ...)`)
✅ GUIDELINE_APPROVED markers

### Need To Add
⬜ **SQL DDL statements** - `CREATE TABLE`, `INSERT INTO`, etc.
⬜ **Line continuations** - `"string" \`
⬜ **Comments** - Better filtering of comment-only lines

### Questionable (Currently Approved, May Need Review)
❓ JSON field names in parsing - `strstr(..., "field:")`
❓ JSON literals - `snprintf(..., "{...}")`

These are context-dependent. Should they be in headers or inline?

## Recommended Implementation Plan

### Phase 1: Quick Fixes (15 minutes)
1. Add SQL pattern to auto-approval (one line change)
2. Add line continuation pattern (one line change)
3. Test on current codebase

### Phase 2: Comment Stripping (30 minutes)
1. Write proper comment-stripping awk function
2. Add preprocessing step before string detection
3. Test on current codebase

### Phase 3: Documentation (15 minutes)
1. Document all auto-approved patterns in header comment
2. Add examples of each pattern
3. Explain when to use GUIDELINE_APPROVED

### Phase 4: Validation (15 minutes)
1. Run on current codebase
2. Verify 162 → ~20-30 unapproved
3. Manually review remaining unapproved strings
4. Document any new patterns found

**Total time: ~75 minutes**

## Expected Results

**Current:**
- Total: 972 strings
- Approved: 810
- Unapproved: 162 (❌ Too high - false positives)

**After fixes:**
- Total: 972 strings
- Approved: ~940-950
- Unapproved: ~20-30 (✅ Reasonable - actual issues to review)

**False positive reduction: ~85%**

## Testing Plan

1. **Before changes:** Save current unapproved list
2. **Apply Fix 1:** SQL patterns → Expect ~40-50 fewer unapproved
3. **Apply Fix 2:** Line continuations → Expect ~20-30 fewer unapproved
4. **Apply Fix 3:** Comment stripping → Expect ~60-80 fewer unapproved
5. **Manual review:** Check remaining ~20-30 strings

**Validation:**
- No false negatives (approve things that SHOULD be in headers)
- Significantly fewer false positives (flag comments, SQL, etc.)
- Remaining unapproved list is actionable

## Questions for Casey

1. **SQL constants:** Approve inline or require extraction to headers?
   - Current thinking: Inline is fine for SQL DDL (it's configuration, not code)

2. **Multi-line strings:** Always require `static const char*` or allow inline multi-line?
   - Current: Already approved if assigned to variable

3. **JSON literals:** Approve inline JSON field names or require headers?
   - Examples: `"content"`, `"ci_id"`, `"timestamp"`
   - These appear in parsing code frequently

4. **Implementation approach:** Quick patches (Option A) or proper rewrite (Option B)?
   - Recommendation: Option A now (15 min fix), Option B later if needed
