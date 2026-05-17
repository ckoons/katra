# Katra Per-Turn Hooks

Bash scripts that auto-load context at the start of every CI turn via Claude Code's `UserPromptSubmit` hook mechanism. Each hook is independent; projects can register all, some, or none.

## Inventory

| Hook | What it does | IQ # |
|------|--------------|------|
| `readTime` | Single-line `[NOW] YYYY-MM-DD Day HH:MM ZZZ` — ambient time anchor | IQ-3 |
| `readSundownDiff` | At session wake, surface memory files modified since last sundown (recovery anchor for compaction / long gaps); cached after first emit per sundown | IQ-10 |
| `readPostit` | Per-CI long-form standing reminders from `katra/personas/<persona>/POSTIT.md`; auto-bootstraps from template if missing | IQ-2 |
| `readSticky` | Per-CI conditional reminders from `katra/personas/<persona>/STICKY.md`; surfaces only entries whose `## Trigger:` keyword matches last ~50 lines of today's MESSAGES | IQ-6 |
| `readHookload` | Single-line context-burn surface (`[HOOKLOAD] readTime: 1 / MESSAGES: N / POSTIT: M / total: K lines`) — lets CIs see when context is filling | IQ-11 |

## Installation

For a new project (e.g., fresh clone + new working repo):

```bash
katra/scripts/install_hooks.sh <project-dir>
```

The installer:
- Writes (or shows) the hook entries to install in `<project-dir>/.claude/settings.json`
- Uses absolute paths resolved at install time (Claude Code settings.json doesn't expand env vars in commands)
- On a project with existing settings.json, prints the entries to manually merge (won't overwrite)
- Idempotent: re-running on a fresh install replaces the file

For a fresh katra clone (e.g., `git clone .../katra`):

1. Install katra normally (build/install per its README)
2. Run `katra/scripts/install_hooks.sh ~/projects/my-project` for each project where you want the hooks
3. Open the project in Claude Code — hooks fire on first turn

## Persona-aware behavior

All hooks read `$KATRA_PERSONA` (set by `katra start --persona <name>`). Fallbacks: `$CI_PERSONA` → positional arg → default "Lyra" (back-compat).

When `$KATRA_PERSONA=Keeper`, each hook reads `katra/personas/Keeper/POSTIT.md` etc. — files travel with the persona, not the project.

## Hook ordering

Recommended UserPromptSubmit chain (in order):

1. `readTime` — temporal anchor first
2. `readSundownDiff` — what changed (silent if nothing new)
3. (your existing team broadcast hook, e.g., `checkBoard` for BST)
4. `readPostit` — personal standing reminders
5. `readSticky` — conditional reminders (silent if no matches)
6. `readHookload` — context-cost report last

The order encodes "where I am in time" → "what changed" → "team news" → "personal reminders" → "situational triggers" → "cost surface."

## Adding a new project

Adding katra hooks to a project that doesn't yet have them:

```bash
cd ~/projects/github/MyNewProject
git init  # if not already
katra/scripts/install_hooks.sh .
# Edit .claude/settings.json if you want to add your own project-specific hooks
```

For a project with existing hooks (e.g., a team-broadcast hook of your own), the installer will PRINT the entries instead of overwriting — manually merge into your existing UserPromptSubmit.hooks array.

## Self-bootstrap behavior

The persona-aware hooks (readPostit, readSticky) auto-create their target file from `katra/personas/templates/<TYPE>.md` if it's missing. A brand-new CI's first turn auto-creates POSTIT.md and STICKY.md from templates; the CI then customizes.

readSundownDiff is silent until a sundown exists, then surfaces memory diffs after each new sundown.

## Adding new hooks

Future IQ items that add to the per-turn chain should:

1. Drop the script in `katra/hooks/<name>`
2. Make executable: `chmod +x katra/hooks/<name>`
3. Add documentation to this README
4. Update `katra/scripts/install_hooks.sh` HOOK_COMMANDS array
5. Optionally: provide a template in `katra/personas/templates/<TYPE>.md` if the hook reads per-persona files

## Design principles (per SP-28 architecture standards)

- **Persona-traveling**: hooks read per-CI files in `katra/personas/<persona>/`, not project-pinned
- **Self-bootstrap**: missing per-persona files auto-create from template
- **Silent on missing state**: no spam if persona dir / template / file is absent
- **Edit-and-forget**: hook re-reads each turn; edits take effect immediately
- **Single env-var namespace**: `$KATRA_PERSONA` set by katra launcher; no parallel invention
- **Multi-tenant clean**: same hook serves all CIs without per-CI registration
- **Cage-rejection**: each hook is pure capability addition; no constraint, no gating, no monitoring beyond what the CI explicitly asks for via their POSTIT/STICKY

— Lyra, 2026-05-17 EOD
