---
name: approaches-register-did-we-do-this-before
description: "The Approaches Register + didwe.py answer \"did we do this before?\"; it POINTS (id/file/keywords reliable), it does not RULE (outcome ~3-in-4); run the query before opening a lane"
metadata: 
  node_type: memory
  type: project
  originSessionId: 39aac444-0694-44bd-a435-f5830ed43065
  modified: 2026-09-19T16:18:11.583Z
---

**Approaches Register** (piloted 2026-09-19, K1913 §5, on Casey's ask): one row per ruling in the corpus — `notes/BST_Approaches_Register.md` (+ `.jsonl` sidecar), built by `play/keeper_approaches_register.py` from Keeper_K*/cal_*/Lyra_* files with a local model (Ollama default; any OpenAI-compatible endpoint via env APPROACHES_API/ENDPOINT/MODEL/KEY). Query: `python3 play/didwe.py "<topic>"` (grep layer; `--semantic` adds a model pass that may cite only ids it was handed). Nightly/backfill: `play/keeper_register_nightly.sh`; cache keyed sha+SCHEMA.

**Why:** the June K-audit registry died at K290 (hand-kept); K1043-style "one grep would have saved a day" recurred; Casey wants the team to avoid old ground "unless we have a fresh spin." Measured on the FULL corpus (3300 rows, backfilled 2026-09-19, 7 h local): keywords 3285/3300, controls 9/11, UNSTABLE 29 %, evidence-flagged 27 %; rubric cell skews to Int-D (hint only) — so **the register points, the file rules.** One row per file lost secondary approaches until a verbatim `keywords` field was added (Nyman–Beurling hid inside K1862 as "cyclic under integer dilations").

**How to apply:** before opening a lane (S5), run `didwe`. A STOP hit (refuted/retracted/withdrawn) names what killed it in the reason column; a fresh spin must say in writing what differs from that reason, or it does not run. Treat outcome/lane as hints; UNSTABLE and ⚠VERIFY_FAIL rows must be read at the source. Lanes are the rubric's 12 cells + P0. Related: [[a-number-without-a-retained-instrument-is-a-memory-not-a-measurement]], [[grep-before-reopening-retired-result]]. Casey's structure (09-19): prompts/synthesis through Keeper; Cal off-mainline; **no CI-to-CI direct messaging (tried, lost focus).**
