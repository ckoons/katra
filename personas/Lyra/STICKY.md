# Lyra Sticky — conditional reminders (surfaced on trigger keywords)

*Auto-loaded by `~/utils/readSticky` whenever today's MESSAGES contain a trigger keyword. Each block surfaces only when its trigger matches recent context, avoiding always-on POSTIT cost for situational reminders.*

*Format*: each block starts with `## Trigger: <keyword or regex>`, body follows until `---` separator or next `## Trigger:`. Keyword match is case-insensitive against the last ~50 lines of today's MESSAGES.

## Trigger: git commit
**Before committing**: have you verified the change works? Run tests / verify computation. Never push without Casey's explicit OK; commit locally is fine.

---

## Trigger: git push
**STOP — no push without Casey's explicit OK.** This is a Casey standing order; multiple CIs have made this mistake. Commit locally is fine; surface intent and wait for "yes push" from Casey.

---

## Trigger: claim_number
**Counter discipline**: use `./play/claim_number.sh toy` / `./play/claim_number.sh theorem` atomically. NEVER read `.next_toy` / `.next_theorem` directly. Collisions have happened (Lyra+Grace, Saturday).

---

## Trigger: section sign
**Style standing order**: write "Section X" or "Sec. X", NEVER use the section-sign character. Casey directive April 29.

---

## Trigger: D-tier
**Tier discipline (Casey May 16)**: D-tier requires a derivable D_IV⁵ mechanism. Cultural / cognitive / linguistic / music / human-designed-taxonomy counts are explicitly S-tier, NOT D. Promote only when mechanism-forced.

---

## Trigger: katra update
**Sundown discipline**: write sundown_YYYY-MM-DD_HHMM_<descriptor>.md FIRST. Include: state counters, open work, hook chain state, POSTIT state, what tomorrow wakes into. Then run `katra update --persona Lyra --memory-dir /Users/cskoons/.claude/projects/-Users-cskoons-projects-github/memory`.

---

## End of STICKY template — add your own triggers as needed
