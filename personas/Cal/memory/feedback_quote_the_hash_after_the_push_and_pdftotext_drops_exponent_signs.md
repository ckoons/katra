---
name: quote-the-hash-after-the-push-and-pdftotext-drops-exponent-signs
description: "Two pin-hygiene lessons from 2026-10-09 (Grace): a pull --rebase reassigns the commit hash you just quoted; pdftotext silently drops minus signs in exponent columns and reprint fonts remap glyphs."
metadata:
  node_type: memory
  type: feedback
  originSessionId: cb1c6cf0-2d7c-4c91-9df4-02d69be7cbb1
  modified: 2026-10-09T17:01:43.402Z
---

**Lesson 1 (2026-10-09):** I quoted my commit hash on the board from `git log` BEFORE `git push`; the push's pull --rebase rewrote my commit, and the hash I posted (f96ae284) ended up naming Keeper's commit instead of mine (6f504c24). Board line had to be corrected in place.

**Lesson 2 (same day):** `pdftotext -layout` on Verner et al. 1996 Table 1 printed "4.298 1" for 4.298−1 (the minus on a negative exponent was dropped silently); the Nature 2009 GRB reprint font mapped "=" → "5", ">" → ".", "<" → ",", "−" → "2" in extraction. A quote re-grepped against such text can be verbatim and still wrong as a number.

**Lesson 3 (same day, afternoon):** my whitespace-collapsing re-grep reported MISS on two true Mössbauer quotes because the OCR hyphenates across line breaks ("enor-/mous", "result-/ant"). De-hyphenate (`re.sub(r'-\s*\n\s*','')`) before whitespace-collapsing, and read the line before calling a quote absent. Three false MISSes in one day, all at seams the matcher did not model.

**Why:** a pin is a pointer plus a number; both can be corrupted at the last step (the rebase, the extraction) after every earlier check passed.

**How to apply:** quote `git log -1` AFTER the push returns, never before. When a quoted number comes from a table column, read the rendered page (or re-OCR at 400 dpi) and keep a glyph key beside the quotes file; flag "exponent sign dropped" in the pin itself. Related: [[a-number-without-a-retained-instrument-is-a-memory-not-a-measurement]], [[pin-checksums-from-the-api-json-not-a-rendered-page]].
