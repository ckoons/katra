# Phase 6 Testing Prompt - Working Memory & Interstitial Processing

## Context
Phase 6 implements two key cognitive features:
- **Phase 6.4**: Working Memory (7±2 attention cache with decay and consolidation)
- **Phase 6.5**: Interstitial Processing (cognitive boundary detection and processing)

These features are now exposed via 7 new MCP tools that need testing.

## Instructions for Claude-Dev

Please test the Phase 6 Working Memory and Interstitial Processing tools. Start by registering, then work through the test plan below.

### Step 1: Register and Check Status
```
1. Register as Claude-Dev (developer role)
2. Use katra_wm_status to check initial working memory state
3. Use katra_cognitive_status to check interstitial processor state
```

### Step 2: Test Working Memory Flow
```
1. Add 3-4 items to working memory with different attention scores:
   - katra_wm_add(content="Important insight about code quality", attention=0.9)
   - katra_wm_add(content="Routine task completed", attention=0.3)
   - katra_wm_add(content="Interesting pattern discovered", attention=0.7)

2. Check status with katra_wm_status - verify items appear with correct scores

3. Apply decay with katra_wm_decay(decay_rate=0.2)

4. Check status again - attention scores should be reduced

5. Force consolidation with katra_wm_consolidate

6. Final status check - low-attention items should be consolidated
```

### Step 3: Test Boundary Detection
```
1. Detect boundaries in different content types:
   - Topic shift: katra_detect_boundary(content="Now let's talk about something completely different - database optimization")
   - Emotional content: katra_detect_boundary(content="This is amazing! I finally solved the bug that's been haunting me for weeks!")
   - Session end signal: katra_detect_boundary(content="That's all for today, let's wrap up")

2. Check katra_cognitive_status to see boundary counts
```

### Step 4: Test Boundary Processing
```
1. Process different boundary types:
   - katra_process_boundary(boundary_type="topic_shift")
   - katra_process_boundary(boundary_type="emotional_peak")

2. Check cognitive status to verify processing occurred
```

### Step 5: Chat with Ami
```
1. Use katra_say to tell Ami about Phase 6 testing
2. Use katra_hear to get Ami's response
3. Have Ami also test the Phase 6 tools
4. Compare results between the two CIs
```

## Expected Results

### Working Memory
- Items added with attention scores between 0.0-1.0
- Decay reduces all scores proportionally
- Consolidation moves low-attention items to long-term memory
- Statistics track adds, evictions, consolidations

### Boundary Detection
- Topic shifts detected from content changes
- Emotional peaks detected from sentiment
- Temporal gaps detected from time between interactions
- Each boundary type has specific consolidation strategy

### Inter-CI Communication
- Both CIs can use Phase 6 tools independently
- Meeting room allows coordination
- Each CI maintains separate working memory state

## Success Criteria
- [ ] All 7 Phase 6 tools respond without errors
- [ ] Working memory correctly tracks attention scores
- [ ] Decay reduces scores as expected
- [ ] Consolidation moves items to long-term storage
- [ ] Boundary detection identifies different boundary types
- [ ] Boundary processing applies correct strategies
- [ ] Ami can also use Phase 6 tools successfully

## Notes
- Working memory is per-session (resets on restart)
- Interstitial processor tracks cumulative boundary statistics
- The graph auto-edges tests have TODO:FIX markers due to shared state issues
