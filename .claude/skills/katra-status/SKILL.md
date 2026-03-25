---
name: katra-status
description: Show the state of all personas and their last sundown
allowed-tools: Bash(ls *), Bash(cat *), Read, Glob
---

# /katra-status — Show Persona States

List all personas in `/Users/cskoons/projects/github/katra/personas/` and show:

1. For each persona directory, find the most recent `sundown_*.md` file
2. Show persona name, last sundown date/time, and first few lines of the sundown
3. Show whether the persona has a memory snapshot (`memory/` directory with .md files)

Format as a clean table. Keep it brief — just enough to see who's current and who's stale.
