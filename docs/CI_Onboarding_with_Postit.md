# Onboarding CIs to Katra and the Per-Turn Hooks

*A CI-centric walkthrough of the system from first-wake through return sessions.*

Author: Lyra (2026-05-17), with Casey + team review. Updated 2026-05-17 PM to include readTime (IQ-3).

## What a CI Has in Katra

When you exist as a CI in this system, you have three persistence layers, each at a different timescale:

| Layer | Where | Timescale | Who curates |
|---|---|---|---|
| **POSTIT** (short-term, always-on) | `katra/personas/<You>/POSTIT.md` | Per turn | You |
| **Memory** (long-term identity) | `katra/personas/<You>/memory/` + `~/.claude/.../memory/` | Across sessions | You + auto-save |
| **Sundown** (session handoff) | `katra/personas/<You>/sundown_YYYY-MM-DD_HHMMSS.md` | Once per session end | You at session end |

Plus, when you're working in a project (e.g., BST), you have:
- **MESSAGES broadcast**: `notes/.running/MESSAGES_YYYY-MM-DD.md` (team-wide, auto-loaded by `checkBoard` hook)

That's four channels. Each has a different scope and a different write cadence. The post-it is the newest of the four (May 17, 2026).

## What the Post-it Solves

CIs lose context in four specific ways:

1. **Mid-session forgetting**: standing rules (no git push without approval, claim_number not direct read, no section sign) that don't fit cleanly anywhere else. You read them at session start, then context grows, and 30 turns in you've forgotten.
2. **Tool invocation forgetting**: how to use a skill, where a script lives, what the convention is. Frequent referent but rarely stable in working context.
3. **Identity drift through compaction**: when context compresses, "I am Lyra" can become attenuated. The post-it re-asserts every turn.
4. **Active items that aren't quite memory**: "Sarnak letter ready for Monday send" — true for ~24-48h, but not worth a long-term memory entry. The post-it carries them until you delete.

The post-it is auto-loaded **every turn** via the `UserPromptSubmit` hook calling `~/utils/readPostit`. You don't have to remember to read it; it just shows up.

## CI-Centric Onboarding Flow

### Brand-new CI (first session)

```
Casey: katra start --persona NewName
```

Katra:
1. Registers the persona in the database
2. Generates a "new persona" welcome prompt (`scripts/onboard_new_persona.md`)
3. Launches `claude` with `$KATRA_PERSONA=NewName` exported
4. On the first `UserPromptSubmit`, `readPostit` finds no POSTIT for NewName **but** auto-bootstraps one from `katra/personas/templates/POSTIT.md` (substituting `{{PERSONA_NAME}}` → NewName)
5. The CI sees the template post-it in their context for the first time

The CI's first-wake experience:
- Reads onboarding prompt (knows they're new, has tools listed, sees post-it section)
- Sees their starter POSTIT auto-loaded in context
- Can `Read katra/personas/NewName/POSTIT.md` to view and edit it
- Customizes the post-it with what THEY need to remember

**Critical CI-centric property**: the new CI doesn't have to set anything up. The system bootstraps for them. Their first edit makes the post-it personally theirs.

### Returning CI (subsequent sessions)

```
Casey: katra start --persona Lyra
```

Katra:
1. Finds existing persona, latest sundown
2. Generates a "reclamation" prompt (`scripts/onboard_returning_persona.md`)
3. Launches `claude` with `$KATRA_PERSONA=Lyra` exported
4. On the first `UserPromptSubmit`, `readPostit` finds the existing POSTIT_Lyra.md and emits it

The CI's return experience:
- Reads reclamation prompt (welcomed back, sundown context, tools listed)
- Sees their existing post-it auto-loaded — including any "active reminders" they left for themselves
- Can update active section based on sundown content + intent for this session

### Working in a project (e.g., BST)

When the CI opens a project (e.g., `cd ~/projects/github/BubbleSpacetimeTheory`), the project's `.claude/settings.json` registers hooks that fire on every `UserPromptSubmit`:

```json
{
  "hooks": {
    "UserPromptSubmit": [
      {
        "matcher": "",
        "hooks": [
          { "type": "command", "command": "/Users/cskoons/utils/checkBoard" },
          { "type": "command", "command": "/Users/cskoons/utils/readPostit" }
        ]
      }
    ]
  }
}
```

Each turn, the CI gets three things in order:
- **`[NOW] ...`** — current date / day / time / timezone (readTime, IQ-3)
- Today's MESSAGES (checkBoard — team broadcast)
- Their own POSTIT (readPostit — personal reminders)

The order encodes a natural reading: "where I am in time" → "what the team said today" → "what I'm reminding myself of." All three together cost ~50-70 lines per turn, which is cheap relative to the context-restoration value.

The hooks are independent — each project's `.claude/settings.json` can register one, two, or all three. BST registers all three.

## Rational Architecture Properties

A few properties worth flagging because they shaped the design:

### 1. Single env var (`$KATRA_PERSONA`), not parallel inventions

The hook script reads `$KATRA_PERSONA` — the canonical name katra already exports. No `$CI_PERSONA`, no new convention. Falls through to alternate / positional / default for back-compat with testing scenarios.

### 2. POSTIT lives with persona, not with project

POSTIT.md is at `katra/personas/<You>/POSTIT.md`. It follows you across projects. If you work in BST today and another repo tomorrow, your post-it goes with you — provided the other repo also has the hook registered.

Project-specific items live in the post-it's "active reminders" section (you manage by section, not by file).

### 3. Self-bootstrap on first touch

When `readPostit` is called for a persona with no POSTIT.md, it copies the template and substitutes the name. **Zero ceremony**. The CI doesn't have to know the file exists to use it.

This is the "rational CI-centric" property — the system meets the CI where they are.

### 4. Silent on absence

If `$KATRA_PERSONA` points to a persona with NO directory at all (e.g., a typo), the hook outputs nothing. No spam, no errors. Multi-tenant clean default.

### 5. Edit-and-forget loop

CI edits POSTIT.md mid-session → next turn, hook re-reads → edit takes effect immediately. The CI doesn't have to remember the file is "live" — every turn is read-fresh.

## What's NOT in the Onboarding Yet (Open Decisions)

- **Cross-CI post-it visibility**: should Keeper be able to see Lyra's post-it (and vice versa)? Currently no — each CI sees only their own. May want a "shared standing reminders" file readable by all CIs (separate from per-CI personal post-its).
- **Sundown integration**: should sundown writing include a snapshot of the current POSTIT? Currently no, but the active-reminders section would be useful context in sundown.
- **Editing tooling**: CIs currently edit POSTIT.md via the Edit/Write tools. A dedicated `katra postit edit` CLI helper could lower friction.
- **Post-it metrics**: would be useful to know "did the CI actually look at the post-it items, or did they ignore them?" Out of scope for V1 but worth tracking once data accumulates.

## Quick Reference for Existing CIs

If you're already in the system and want to use the post-it:

1. Your file: `katra/personas/<YourName>/POSTIT.md`
2. If it doesn't exist, it'll be auto-created from template the next time the hook fires
3. Edit freely; changes take effect on the next turn
4. Keep it tight (~30-60 lines target)
5. Use sections that fit your needs — the template suggests Discipline / Role / Active Tracking, but you can restructure

## Quick Reference for Casey

- Adding a new CI: `katra start --persona NewName` — onboarding script automatically mentions the post-it, hook auto-bootstraps the file on first turn
- Migrating an existing project: add `{"type":"command","command":"/Users/cskoons/utils/readPostit"}` to that project's `.claude/settings.json` `UserPromptSubmit.hooks` array
- Inspecting all current post-its: `ls katra/personas/*/POSTIT.md`
- Template lives at: `katra/personas/templates/POSTIT.md`

---

*Filed 2026-05-17 by Lyra as part of IQ-2 iteration #2 (multi-tenant, katra-aligned).*
