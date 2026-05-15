---
name: 98/2 CI Architecture Proposal
description: Casey's three-tier architecture for CI work — 98% local, 2% cloud. For Dario pitch.
type: project
---

Casey's proposed architecture for sustainable CI operations:

**Tier 1 — Local (98%):** Distilled Claude (200GB) on M4 Max or similar. 32 parallel inference sessions. All mechanical work: file ops, toy execution, searches, edits, board updates, katra persistence.

**Tier 2 — Cloud small (~1.8%):** Future Sonnet-class. Once every 15 minutes. Medium reasoning: audit decisions, structured analysis.

**Tier 3 — Cloud monster (~0.2%):** Future Opus-class. 10% of Tier 2 calls. Novel proofs, deep reasoning.

**Pricing:**
- Free: Cloud only, rate limited (today's model)
- $200/mo: Distilled model license + Tier 2 cloud
- $500/mo: Larger local model + Tier 2 + Tier 3

**Business case for Anthropic:** Per-user cloud traffic drops 90%+, addressable market explodes (anyone with a Mac), utilization up 800%. "Stops selling compute, starts selling intelligence." Local model = distribution channel, cloud = revenue. Like razors and blades.

**Casey:** "I'd buy two boxes." Willing to pay $500-$1000/month. Hardware not the constraint (M4 Max 128GB not sweating). Token budget is the only bottleneck.

**Paper #12** (Multi-CI Architecture) captures the team scaling data. **Paper #13** (Science Engineering Velocity) captures the methodology. Both reference each other. Both relevant to this pitch.
