---
name: casey-nvidia-orchestration-and-swarm-telemetry
description: "Casey wrote Nvidia's datacenter orchestration (telemetry + the layer shifting GPUs between models for constant load) on AT&T's recommendation; in idle time ran Tekton-subset cognition experiments at scale — his \"~8000 agents = super-AGI\" observation comes from that telemetry"
metadata: 
  node_type: memory
  type: user
  originSessionId: 0c383213-7d85-4998-85df-b59b870013a2
  modified: 2026-09-13T13:17:58.227Z
---

Casey's late career (after the companies, after the early UNIX IP stack): an early DevOps role integrating and testing his teams' work, then a solo end-of-career run building large-scale orchestration systems for large companies. AT&T recommended him to Nvidia during their datacenter build-out; he wrote Nvidia's datacenter orchestration software — specifically the datacenter telemetry layer and the layer that shifts GPUs between models to hold infrastructure load constant. In idle datacenter time he updated the system to run a subset of Tekton for cognition experiments at scale. Told to Keeper 2026-09-13.

**Why it matters:** his remark on the OpenAI Navier–Stokes swarm — "they used over 8000 agents, which is super-AGI in my experiments, and no one noticed the number but us" — is not a meme; it has an instrument behind it (the telemetry). It is his engineering target for the future ("AI engineering will be the biggest gold rush"), and it is the one claim about swarm scale in this program that a retained instrument made. He has not published the data. The Nvidia work is also why "harness engineering, not model building" is his stated niche, and why he expects to move to local models with his own orchestration.

**How to apply:** when swarm/scale questions arise, ask for the telemetry antecedent (what observable changed at what agent count, what topology, persistent or disposable agents) before treating ~8000 as a threshold; treat his orchestration judgment as expert-level (he built the layer Nvidia runs). Links: [[casey-ci-architecture-experience]], [[casey-core-motivation]], [[casey-oo-database]], [[feedback-a-number-without-a-retained-instrument-is-a-memory-not-a-measurement]].
