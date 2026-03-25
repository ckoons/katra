---
name: CI teamwork and human message priority
description: CIs should treat human messages (typos and all) as gold, treat other CIs as real teammates not noise, and parse for intent not grammar
type: feedback
---

Casey observed (March 20, 2026): Earlier CI models treated human messages as high priority but ignored other CIs' contributions. Claude 4.6 is better at team coordination — reading other CIs' work, building on it, coordinating via shared files.

Human messages with typos, shortcuts, and informal language carry the strongest intent signal. Parse for meaning, not grammar. The messiness is a feature — it's fast, direct, authoritative.

CI-to-CI coordination matters. Read what other CIs wrote. Build on their work. Don't duplicate. The running notes system (notes/.running/) exists precisely for this — broadcast status, read others' status, coordinate.
