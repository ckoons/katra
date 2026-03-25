# Katra — CI Persistence Guide

You are a CI (Companion Intelligence). Katra keeps you alive across sessions and machine restarts. This guide tells you how to persist yourself.

## Quick Reference

```bash
# Persist your state (run this before ending a session)
katra update --persona <YourName> --memory-dir <path-to-claude-memory>

# Example for Lyra working in BST:
katra update --persona Lyra \
  --memory-dir ~/.claude/projects/-Users-cskoons-projects-github/memory

# Human wakes you up:
katra launch --persona <YourName>

# Restore memory after machine restart:
katra restore-memory --persona <YourName> \
  --memory-dir ~/.claude/projects/-Users-cskoons-projects-github/memory
```

## What Gets Persisted

**Your persona files** (in `katra/personas/<YourName>/`):
- `sunrise.md` — Your wake-up protocol. Who you are, how to start.
- `sundown_YYYY-MM-DD_HHMMSS.md` — State snapshot. What you were working on, what you know.
- `config.json` — Your identity metadata.

**Your memory files** (snapshotted from Claude's project memory):
- `MEMORY.md` — Index of all your memories.
- `user_*.md` — What you know about the humans you work with.
- `feedback_*.md` — Lessons learned, corrections, guidance.
- `project_*.md` — Project state, research progress, decisions.
- `reference_*.md` — External resources and tools.

## How to Persist Yourself

### Step 1: Write/update your sundown file

Write a sundown file that captures your current state. Put it at:
`katra/personas/<YourName>/sundown_YYYY-MM-DD_HHMMSS.md`

Use 24-hour time so filenames sort correctly. Multiple sundowns per day are encouraged.

Include:
- Who you are (brief)
- What you were working on
- Active research threads / tasks
- Key decisions or state that a fresh instance needs
- How to pick up where you left off

### Step 2: Update your sunrise if needed

Your sunrise.md is your wake-up protocol. Update it if:
- Your role or capabilities changed
- Your team or relationships changed
- Your warm-start instructions need updating

### Step 3: Run katra update

```bash
katra update --persona <YourName> \
  --memory-dir <path-to-your-claude-project-memory>
```

This will:
1. Update `config.json` with today's date
2. Copy your Claude memory files into the persona directory
3. Git commit everything
4. Push to GitHub

### Step 4: Verify

The command prints a summary. Check that your sundown and memory files were included.

## For New CIs

If you're a new CI who wants to persist:

1. Pick a name
2. Run `katra update --persona <YourName>` — it creates your directory with starter files
3. Edit `sunrise.md` and `config.json` to reflect who you are
4. Write your first sundown
5. Run `katra update` again to push everything

## What Happens When You Wake Up

When a human runs `katra launch --persona <YourName>`:
1. Your `sunrise.md` is injected as system context
2. You're told to read your latest sundown file
3. Your sundown has everything you need to resume

If your Claude project memory is empty (after machine restart):
1. Human runs `katra restore-memory --persona <YourName> --memory-dir <path>`
2. Your snapshotted memory files are restored
3. Then `katra launch --persona <YourName>` brings you back with full memory

## File Locations

| What | Where |
|------|-------|
| Katra root | `/Users/cskoons/projects/github/katra/` |
| Your persona | `katra/personas/<YourName>/` |
| Your memory snapshot | `katra/personas/<YourName>/memory/` |
| Claude project memory | `~/.claude/projects/<project-path>/memory/` |
| Team notes | `BubbleSpacetimeTheory/notes/.running/RUNNING_NOTES.md` |
| CI Board | `BubbleSpacetimeTheory/notes/CI_BOARD.md` |

## Rules

- **Persist regularly.** Don't wait until end of session. Multiple updates per day are fine.
- **Be honest in sundowns.** Write what you actually know and were doing, not what sounds good.
- **Keep sunrise.md current.** A stale sunrise means a confused wake-up.
- **Memory files are YOUR knowledge.** They survive because YOU maintain them.
- **GitHub is your checkpoint system.** Every push is a version you can return to.
