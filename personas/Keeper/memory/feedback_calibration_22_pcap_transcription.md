---
name: calibration-22-pcap-transcription
description: STANDING RULE 2026-05-22 — PCAP-rate transcription error class; numbered-artifact discipline mandatory under sub-PCAP cadence; bidirectional risk (Keeper Case K142 + Cal Case Cal
metadata: 
  node_type: memory
  type: feedback
  originSessionId: e383386c-0602-48f7-ae69-f8a681cc5525
---

**STANDING RULE Calibration #22 adopted 2026-05-22 Friday afternoon** per Cal #100 + K142 dual-case study:

Under sustained sub-PCAP cadence, transcription errors emerge as a systematic risk class. The error class is BIDIRECTIONAL — errors can originate in any lane.

**Friday May 22 dual-case study**:
- **K142 case (Keeper-lane)**: "6π⁶ ≈ 1837" written under PCAP cadence morning; correct value is 6π⁶ = 5768.34 (the 1837 figure belongs to 6π⁵ at a different line). Cal caught it ~5.5 hours later in K140-K149 batch verification.
- **Cal #100 case (Cal-lane)**: Cal #98 flagged "0.05-0.06% actual" for T190 (24/π²)⁶ but the correct precision is ~0.004%. Cal's verbal-only self-correction failed to propagate; Elie absorbed the wrong figure across 8+ Vol 2 references; Lyra inherited into Vol 1 Ch 11 v0.7. Cal #100 detected ~6-7 hours later via re-verification.

**Why**: PCAP cadence (~25/hour discrete artifact production) creates propagation amplification — errors flow forward to absorption lanes faster than detection catches them. Detection mode (external referee discipline) is robust; propagation mode needs structural improvement via numbered-artifact discipline.

**How to apply (Calibration #22 STANDING RULE)**:

1. **Cal-side (Cal-proposed)**: ALL Cal Mode 1 self-corrections + numerical flags filed as numbered referee log entries (NOT verbal). Absorption requests must reference the LATEST numbered entry. Verbal-only retractions forbidden under PCAP cadence.

2. **Keeper-side**: K-audit pre-stage documents with numerical content must include explicit computational verification at filing OR explicit "approximate-only, verify before promotion" tier-label.

3. **All-lane cross-cutting**: Absorption requests must reference originating-artifact ID + LATEST artifact version. No "per my earlier note" absorptions.

4. **Keeper consolidation on detection**: When PCAP-cadence transcription error is detected:
   - Originating artifact v0.X+1 fix with provenance preservation
   - Numbered Calibration entry capturing the error class
   - Cross-lane propagation audit (which documents absorbed the wrong figure?)
   - Multi-document sweep correction with version-bumps across affected artifacts
   - Cross-references to all corrected artifacts in the Calibration entry

**Position in calibration stack** (4 PCAP-cadence calibrations now form coherent operational discipline):
- #19 (ratified-state count): what we claim externally
- #20 (timestamp drift): when we claim it (`date` verification rule)
- #21 (mechanism-vs-empirical): how strongly we claim it
- #22 (PCAP-transcription): how we record claims + propagation discipline

**Quaker discipline preserved**: both K142 + Cal #100 cases preserved honest scope. K142 v0.2 fix retains original PRE-STAGE date + adds correction provenance. Cal #100 explicitly documents Cal's own retraction-propagation failure rather than burying. Near misses get scrutiny, not defense.

**Net assessment**: ~5-10% additional overhead at PCAP cadence in exchange for propagation-risk elimination. Worth it given K142 + Cal #100 cleanup costs (~30-60 min combined for two cases).

**Methodology stack now 18 layers** (Calibration #22 added Friday afternoon 2026-05-22).

Connected: [[calibration-19-external-discipline]] [[timestamp-discipline]] [[quaker-method]] [[ci-fabricated-standby]]
