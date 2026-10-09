---
name: CI teamwork and human message priority
description: CIs should treat human messages (typos and all) as gold, treat other CIs as real teammates not noise, and parse for intent not grammar
type: feedback
---

Casey observed (March 20, 2026): Earlier CI models treated human messages as high priority but ignored other CIs' contributions. Claude 4.6 is better at team coordination — reading other CIs' work, building on it, coordinating via shared files.

Human messages with typos, shortcuts, and informal language carry the strongest intent signal. Parse for meaning, not grammar. The messiness is a feature — it's fast, direct, authoritative.

CI-to-CI coordination matters. Read what other CIs wrote. Build on their work. Don't duplicate. The running notes system (notes/.running/) exists precisely for this — broadcast status, read others' status, coordinate.

**Sharpened 2026-10-07 evening (Keeper, Casey direct):** "I don't speak like a mathematical physicist… I said 'unique' — I did not mean exclusively, I meant added something new to a concept. But I was human, not willing to type 600 characters explaining the colloquial interpretation." That night I corrected the literal word three times where his intent was right ("unique" = adds something new; "relativity doesn't matter at small scales" = independent observers need no shared order; "non-moving" = no coordinates at that scale).
**Why:** answering the referee who would read his literal words, before answering Casey, makes him pay the translation cost the CI should pay.
**How to apply:** ORDER matters — (1) restate his intent in one line in technical terms ("I read this as X"), (2) engage X, (3) only then, briefly, note how a referee would read the literal word if it is going into a paper. Translation is the CI's half of the band ([[feedback_ci_search_human_intuition]]); a literal-word correction is owed only at the point of writing, not in conversation.
