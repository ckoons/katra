---
name: feedback_read_the_tool_before_ruling_on_the_tool_a_remembered_fix_may_name_a_remedy_it_already_has
description: Before proposing a fix for a script/tool/counter/config, READ it — a remedy recalled from memory may already be present, and the real defect elsewhere
metadata:
  node_type: memory
  type: feedback
  originSessionId: 206b44d0-388f-4035-977c-310e7809ce46
---

**Read the tool before ruling on the tool.** (Keeper K1768a, 2026-08-21.) When a governance/infra fix is proposed from memory, it can name a remedy the instrument *already has* — and miss the real defect. Concrete case: K1768 ruled "claim_number.sh is non-atomic → add a counter-lock." Reading the script first showed it **already** acquires an atomic `mkdir` lock; the actual causes were (a) THREE divergent `.next_toy` counter files in different directories (the lock guards only one), and (b) an `audit` subcommand that `set -e`-dies after the first duplicate, so it can't enumerate collisions. Both invisible from memory; both obvious on read.

**Why:** a tool is a primary source about itself. A remembered model of the tool is a citation — it can be stale or wrong the same way a mistranscribed corpus line is. This is the [[feedback_re_derivation_sheds_scope_grep_before_registering_to_inherit_the_caveats]] / "the corpus out-argues fresh analysis" discipline pushed down one level: onto scripts, counters, config, CI infrastructure. The [[feedback_bst_primary_standard_physics_evaluation_only]] hierarchy analog — **structural pin (read the code) > primary source > remembered model.**

**How to apply:** before writing any ruling that touches a script/counter/config/pipeline, open it and confirm the defect is where you think. Then respect ownership — diagnose fully, but if it's another CI's domain (play/ = Elie), route the reproducible diagnosis rather than editing their tool or the gitignored "sacred" counters yourself. Related: [[feedback_an_instrument_built_from_N_instances_covers_only_those_N_classes_stress_test_off_origin]] (an instrument's stated behavior and its actual behavior can diverge — verify on the object in front of you, not the class you remember).
