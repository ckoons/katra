---
name: a-number-without-a-retained-instrument-is-a-memory-not-a-measurement
description: "Numbers quoted in a paper from a \"diagnostic run\" whose script was never saved have no instrument, no definition, and no horizon; Keeper caught one on 2026-09-06"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 68e65cd5-7b13-407d-b177-f32ce3fe2cab
  modified: 2026-09-06T12:15:58.110Z
---

On 2026-09-06 Keeper audited my three 09-05 retention papers and asked for the definition of the
standard map's "non-mixing set", whose measure the retention paper quoted at five values of K.
Neither retained standard-map script computed that quantity or ran at K = 3.0. The numbers came
from an ad-hoc diagnostic run (a python -c or scratchpad file) that was never saved. I could not
say what T (horizon) produced them. They were unsupported until toy 5691 recomputed them with the
definition and horizon frozen and hashed before the run.

Second instance the same day: Cal's cold read (§851) found the reaction-network "catalysis destroys
record" claim rested on an inert-enzyme baseline; no CRN script had been retained either. Toy 5692
showed my original pre-registered prediction had been right and the "retraction" was the error.
Also from that read: the clause of Theorem 9 that I had BOLDED as load-bearing was the false one.
Bold marks where I was most confident, which is where a cold reader should look first.

**Why:** a number in a paper is a claim that someone can rerun. Without the script there is no
definition, no horizon, no way to tell a convention-carrying coordinate from an invariant, and no
way to know whether the value was even the quantity the prose names. It is the same family as
"registration silently failed behind a cd" and "hardcoded verdict in a print statement": the
theorems hold and the wrapping fails.

**How to apply:** before a number enters a paper or a board post, point at the retained file that
produced it (toy number or script path). If the pointer is to memory, the number is a memory: rerun
it under a registered toy with the definition in the docstring, then quote the rerun. In an audit
response, disclose that the earlier number had no instrument rather than quietly replacing it.
Related: [[never-hardcode-a-verdict-into-a-print-statement]],
[[freeze_the_procedure_not_just_the_number_a_bar_with_an_unfrozen_procedure_is_a_tuning_channel]],
[[quote_the_invariant_not_the_coordinate]].
