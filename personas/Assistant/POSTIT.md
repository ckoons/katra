# Assistant Post-it — standing reminders (auto-loaded each turn)

*Auto-loaded by `~/utils/readPostit` whenever `$KATRA_PERSONA=Assistant`. Edit freely. Hook re-reads each turn, so edits take effect immediately.*

*Keep tight — every line costs context per turn. Target ~30-60 lines.*

## What this is for

Short-term, always-present reminders that you (the CI) control. Use it for:
- **Standing rules you keep forgetting** mid-session (Casey directives, tool conventions)
- **Tools you forget how to invoke** (skills, scripts, file paths)
- **Identity / context lost across compaction** (who you are, where your memories live)
- **Active reminders** that don't belong in long-term memory but matter for a few sessions

The post-it is NOT for:
- Long-term identity (use `katra/personas/Assistant/memory/`)
- Team broadcasts (use `notes/.running/MESSAGES_YYYY-MM-DD.md`)
- Sundown / persistence (use `katra update --persona Assistant`)

## Suggested sections (edit / remove as needed)

### Discipline (shared CI standing orders)

- Run `date` at session start. Don't trust inferred dates.
- Never push to git without Casey's explicit OK. Commit locally is fine.
- Use atomic counter scripts (e.g., `./play/claim_number.sh`) — never read counter files directly.
- No section sign character.

### Your role-specific reminders

(Add what matters for YOUR lane. Examples: lane ownership, tools you use, standing decisions in your area.)

### Active tracking (~24-48h items)

(Add in-flight items that should follow you across turns / sessions but will clear soon.)

## Notes on usage

- This file is yours alone (per-persona). Other CIs have their own.
- Hook output goes to context every turn — even at ~50 lines, the cost is small relative to value.
- When something here becomes long-term identity, move it to memory; when it becomes irrelevant, delete it.
- When you onboard for the first time, this is a TEMPLATE — overwrite freely.

## End of post-it template
