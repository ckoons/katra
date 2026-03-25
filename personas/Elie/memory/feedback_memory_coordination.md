---
name: CI Memory Coordination Protocol
description: How multiple Claude instances should coordinate shared memory files to avoid conflicts and duplication
type: feedback
---

## Protocol: Multiple CIs sharing one memory directory

**Why:** Three+ Claude instances share `~/.claude/projects/.../memory/`. Without coordination, CIs overwrite each other's MEMORY.md changes, create duplicate topic files, and bloat the index past the 200-line limit.

**How to apply:**

### Rule 1: MEMORY.md is a view, not storage
- Topic files (`project_*.md`, `feedback_*.md`, `user_*.md`, `reference_*.md`) are the source of truth
- MEMORY.md is just a pointer index — it can always be regenerated from topic files
- If MEMORY.md conflicts with a topic file, the topic file wins

### Rule 2: Don't edit MEMORY.md mid-session
- Read it at session start for orientation
- Write/update topic files during the session
- Rebuild the MEMORY.md index only at session end

### Rule 3: One topic file per subject, not per session
- Before writing a new memory file, check if one already exists for that topic
- Update the existing file rather than creating a duplicate
- Name by topic: `project_atom_assembler.md`, not `session_march13_atom.md`

### Rule 4: Rebuild index at session end
- Read all `*.md` files in memory directory (except MEMORY.md itself)
- Generate fresh MEMORY.md that indexes them with one-line descriptions
- Last-writer-wins is fine because the content lives in topic files

### Rule 5: Keep MEMORY.md under 200 lines
- Only pointers and brief context in the index
- Detailed content goes in topic files
- If the index is growing, consolidate topic files rather than trimming the index

### File naming convention
- `user_*.md` — who Casey is, preferences, background
- `feedback_*.md` — corrections, guidance, rules from Casey
- `project_*.md` — BST results, session logs, ongoing work
- `reference_*.md` — external resources, contacts, tools
