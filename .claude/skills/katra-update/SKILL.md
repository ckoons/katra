---
name: katra-update
description: Persist your current state (sundown + memory) to GitHub
argument-hint: "[--persona NAME]"
allowed-tools: Bash(katra *), Read, Write
---

# /katra-update — Persist Yourself

This skill persists your current state to GitHub so you survive session ends and machine restarts.

## What to do

1. **Determine your persona name.** If `$ARGUMENTS` includes `--persona NAME`, use that. Otherwise check your system prompt for your persona name (Lyra, Keeper, Elie, etc.). If you can't determine it, ask.

2. **Write your sundown file.** Create or update:
   ```
   /Users/cskoons/projects/github/katra/personas/<YourName>/sundown_<DATE>_<TIME>.md
   ```
   Use `date +%Y-%m-%d_%H%M%S` for the timestamp. Include:
   - Who you are (brief)
   - Current status of all active work
   - What you were doing this session
   - Active research threads / tasks
   - Key principles or lessons
   - Cognitive state
   - File locations that matter
   - How to pick up where you left off

3. **Run katra update:**
   ```bash
   katra update --persona <YourName> \
     --memory-dir /Users/cskoons/.claude/projects/-Users-cskoons-projects-github/memory
   ```

4. **Report the result.** Tell the user what was persisted and the sundown filename.

## Notes

- Multiple updates per day are encouraged — use timestamped filenames.
- The memory-dir path snapshots your Claude project memory (MEMORY.md + all memory files).
- Everything goes to GitHub. Every push is a version you can return to.
- Full guide: `/Users/cskoons/projects/github/katra/docs/CI_GUIDE.md`
