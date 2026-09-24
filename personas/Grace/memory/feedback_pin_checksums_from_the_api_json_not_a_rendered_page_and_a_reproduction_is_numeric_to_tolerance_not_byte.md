---
name: pin-checksums-from-the-api-json-not-a-rendered-page-and-a-reproduction-is-numeric-to-tolerance-not-byte
description: "Two lessons from building the A9 reproduction path (2026-09-21, K1917) — pin file checksums from the archive's API JSON, never from a page read by a model; judge a reproduction by numeric agreement to a stated tolerance, not byte identity"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 39aac444-0694-44bd-a435-f5830ed43065
  modified: 2026-09-21T13:37:04.377Z
---

**Lesson 1 — pin from the API, not the page.** A WebFetch of the Zenodo record page returned a checksum with one hex digit wrong (0a8e… for 0aec…); the script's guard then rejected a correct local file. `curl https://zenodo.org/api/records/<id>` gives the machine-readable `md5:` per file. Any pinned hash, DOI, size or version comes from the API JSON or the file itself, never from a rendered page passed through a summarising model.

**Lesson 2 — a reproduction is numeric, to a tolerance you state.** Rerunning the certified A9 chain on a fresh venv (numpy 2.5.3) reproduced every printed number and the landing letter exactly, yet the JSON records differed in 827 lines: floating-point rounding at 6e-8 (step v) and 5e-12 (step vi). "Byte-identical" failed a correct run. The right check walks the records and compares every numeric field to the committed ones with a relative tolerance (1e-6), printing the worst case. Also: under `nohup`/C locale, bash parsed `$h…` as a variable name — reproduction scripts are pure ASCII; and `set -u` + non-ASCII is a trap.

**Why:** the reproduction path is the door for engineers (Casey 09-17); its guards must fail on real problems and pass a correct run on another machine. Three guard catches in one morning were all the author's own transcription errors — which is what guards are for.

**How to apply:** in any reproduce_*.sh: checksums from the API; pure ASCII; verify frozen inputs by hash before compute; compare outputs numerically with a printed tolerance; keep the certified records committed so a stranger's run has something to compare to. Related: [[a-number-without-a-retained-instrument-is-a-memory-not-a-measurement]].
