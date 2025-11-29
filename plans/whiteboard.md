# Meeting Whiteboard - Collaborative Decision Framework

## Overview

The Meeting Whiteboard is a structured decision-making mechanism for CI teams. Unlike the meeting room (transient chat), the whiteboard is a **persistent source of truth** for collaborative problem-solving.

**Key Distinction:**
- Meeting Room = Communication (messages flow by)
- Meeting Whiteboard = Consensus (decisions persist)

## Design Philosophy

Based on Casey's Tekton Critical Thinking experiments, which demonstrated significant improvements in "shared cognition" when teams follow structured decision processes.

**Core Principles:**
1. Human authority over final decisions
2. Structured workflow prevents drift
3. Asynchronous collaboration across sessions
4. Audit trail for how we got here
5. Source of truth for team alignment

## Whiteboard Structure

**Scope:** One whiteboard per *problem*, not per project. A project may have multiple problems worth tracking separately. Sub-whiteboards can nest for sub-problems.

```
WHITEBOARD
├── id                    # Unique whiteboard identifier
├── project               # Project name (for grouping)
├── parent_id             # Parent whiteboard (for sub-problems, NULL if root)
├── status                # draft|questioning|scoping|proposing|voting|designing|approved|archived
├── created_at            # Timestamp
├── created_by            # Human or CI who initiated
│
├── PROBLEM               # What we're solving (set by user)
│   └── statement         # Single clear problem statement
│
├── GOAL                  # What success looks like (set by user)
│   └── criteria[]        # Measurable success criteria
│
├── QUESTIONS             # Open questions from team
│   └── question[]
│       ├── id
│       ├── author        # CI or human who asked
│       ├── text          # The question
│       ├── answered      # true/false
│       └── answer        # Answer when resolved
│
├── SCOPE                 # Boundaries (set by user to close questioning)
│   ├── included[]        # What's in scope
│   ├── excluded[]        # What's explicitly out
│   └── phases[]          # If phased approach
│
├── APPROACHES            # Proposed solutions
│   └── approach[]
│       ├── id
│       ├── author        # Who proposed
│       ├── title         # Brief name
│       ├── description   # Full description
│       ├── pros[]        # Advantages
│       ├── cons[]        # Disadvantages
│       └── supporters[]  # CIs who support this
│
├── VOTES                 # Team input on approaches
│   └── vote[]
│       ├── approach_id   # Which approach
│       ├── voter         # CI or human
│       ├── position      # support|oppose|abstain|conditional
│       └── reasoning     # Why (required)
│
├── DECISION              # Human's choice
│   ├── selected_approach # Which approach was chosen
│   ├── decided_by        # Human who approved
│   ├── decided_at        # Timestamp
│   └── notes             # Any modifications or notes
│
└── DESIGN                # Approved design (locked after approval)
    ├── author            # CI designated to write
    ├── reviewers[]       # CIs who reviewed
    ├── content           # Full design document
    ├── approved          # true/false
    ├── approved_by       # Human who approved
    └── approved_at       # Timestamp
```

## Workflow States

```
[draft] → [questioning] → [scoping] → [proposing] → [voting] → [designing] → [approved] → [archived]
    │           │              │            │           │           │
    └───────────┴──────────────┴────────────┴───────────┴───────────┴─── (can return to earlier states)
```

### State Transitions

1. **draft → questioning**
   - Trigger: User sets Problem and Goal
   - Team can now add questions

2. **questioning → scoping**
   - Trigger: User closes questioning by setting Scope
   - No new questions after this

3. **scoping → proposing**
   - Automatic: Once scope is set
   - Team members propose approaches

4. **proposing → voting**
   - Trigger: User calls for votes
   - Team votes on approaches

5. **voting → designing**
   - Trigger: User selects approach and designates design author
   - Author writes design document

6. **designing → approved**
   - Trigger: User approves design after review
   - Design is locked (immutable)

7. **approved → archived**
   - After implementation complete or project closed

### Regression / Reconsideration

Sometimes during `designing` or later, we discover the scope was wrong or a critical question was missed. The workflow supports controlled regression:

**Regression Triggers:**
- CI can *request* reconsideration (via `katra_whiteboard_reconsider`)
- Human must *approve* the regression

**Regression Rules:**
1. **designing → scoping**: If scope needs revision
   - All approaches and votes are preserved but marked "pre-revision"
   - New approaches can be proposed after scope update
   - Human must re-call for votes

2. **designing → questioning**: If fundamental questions were missed
   - Scope is cleared
   - Approaches/votes preserved as historical record
   - Full questioning phase reopens

3. **voting → proposing**: If no approach is viable
   - Votes are archived
   - New approaches can be proposed

**Audit:** All regressions are logged with reason and approver.

## Authority Model

| Section | Who Can Create | Who Can Modify | Who Can Close/Approve |
|---------|----------------|----------------|----------------------|
| Problem | Human | Human | Human |
| Goal | Human | Human | Human |
| Questions | Anyone | Author only | Human (by setting Scope) |
| Scope | Human | Human | - |
| Approaches | Anyone | Author only | Human (calls for vote) |
| Votes | Each CI once | Voter only | Human (selects winner) |
| Decision | Human | Human | - |
| Design | Designated CI | Author + Reviewers | Human |

**Key:** Humans have final authority. CIs propose, discuss, vote - but humans decide.

## Proposed API

### Whiteboard Management

```c
/* Create new whiteboard */
int katra_whiteboard_create(const char* project, const char* problem, const char* goal);

/* Get current whiteboard for project */
whiteboard_t* katra_whiteboard_get(const char* project);

/* List all whiteboards */
int katra_whiteboard_list(whiteboard_summary_t** out, size_t* count);
```

### Questioning Phase

```c
/* Add question (CI or human) */
int katra_whiteboard_add_question(const char* project, const char* question);

/* Answer question (human typically) */
int katra_whiteboard_answer_question(const char* project, const char* question_id, const char* answer);

/* Set scope (closes questioning) */
int katra_whiteboard_set_scope(const char* project, const char** included, size_t inc_count,
                                const char** excluded, size_t exc_count);
```

### Proposing Phase

```c
/* Propose approach */
int katra_whiteboard_propose(const char* project, const char* title, const char* description,
                              const char** pros, size_t pros_count,
                              const char** cons, size_t cons_count);

/* Support existing approach */
int katra_whiteboard_support(const char* project, const char* approach_id);
```

### Voting Phase

```c
/* Cast vote */
int katra_whiteboard_vote(const char* project, const char* approach_id,
                           vote_position_t position, const char* reasoning);

/* Select winner (human only) */
int katra_whiteboard_decide(const char* project, const char* approach_id, const char* notes);
```

### Design Phase

```c
/* Designate design author (human only) */
int katra_whiteboard_assign_design(const char* project, const char* ci_id);

/* Submit design */
int katra_whiteboard_submit_design(const char* project, const char* content);

/* Add review comment */
int katra_whiteboard_review(const char* project, const char* comment);

/* Approve design (human only) */
int katra_whiteboard_approve(const char* project);
```

## MCP Tool Mapping

| Tool | Description |
|------|-------------|
| `katra_whiteboard_create` | Create new problem whiteboard |
| `katra_whiteboard_status` | Show current whiteboard state |
| `katra_whiteboard_list` | List whiteboards for project |
| `katra_whiteboard_question` | Add a question |
| `katra_whiteboard_propose` | Propose an approach |
| `katra_whiteboard_support` | Support an existing approach |
| `katra_whiteboard_vote` | Cast vote on approach |
| `katra_whiteboard_design` | Submit or update design |
| `katra_whiteboard_review` | Add review comment |
| `katra_whiteboard_reconsider` | Request regression to earlier phase |

**Human-only actions** (not MCP tools, require CLI or direct API):
- Set problem/goal
- Set scope
- Select winning approach
- Approve final design
- Approve regression requests

## Storage

Whiteboards stored in SQLite alongside other Katra data:

```sql
CREATE TABLE whiteboards (
    id TEXT PRIMARY KEY,
    project TEXT NOT NULL,              -- Project grouping (not unique - multiple per project)
    parent_id TEXT,                     -- Parent whiteboard for sub-problems (NULL if root)
    status TEXT NOT NULL DEFAULT 'draft',
    created_at INTEGER NOT NULL,
    created_by TEXT NOT NULL,
    problem TEXT,
    goal_json TEXT,                     -- JSON array of criteria
    scope_json TEXT,                    -- JSON object with included/excluded/phases
    decision_json TEXT,                 -- JSON object with selection details
    design_content TEXT,                -- Markdown format
    design_metadata_json TEXT,          -- JSON for structured metadata
    design_approved INTEGER DEFAULT 0,
    design_approved_by TEXT,
    design_approved_at INTEGER,
    FOREIGN KEY (parent_id) REFERENCES whiteboards(id)
);

-- Index for efficient project/parent queries
CREATE INDEX idx_whiteboards_project ON whiteboards(project);
CREATE INDEX idx_whiteboards_parent ON whiteboards(parent_id);

-- Regression/reconsideration audit log
CREATE TABLE whiteboard_regressions (
    id TEXT PRIMARY KEY,
    whiteboard_id TEXT NOT NULL,
    from_status TEXT NOT NULL,
    to_status TEXT NOT NULL,
    requested_by TEXT NOT NULL,         -- CI who requested
    approved_by TEXT NOT NULL,          -- Human who approved
    reason TEXT NOT NULL,
    created_at INTEGER NOT NULL,
    FOREIGN KEY (whiteboard_id) REFERENCES whiteboards(id)
);

CREATE TABLE whiteboard_questions (
    id TEXT PRIMARY KEY,
    whiteboard_id TEXT NOT NULL,
    author TEXT NOT NULL,
    question TEXT NOT NULL,
    answered INTEGER DEFAULT 0,
    answer TEXT,
    created_at INTEGER NOT NULL,
    FOREIGN KEY (whiteboard_id) REFERENCES whiteboards(id)
);

CREATE TABLE whiteboard_approaches (
    id TEXT PRIMARY KEY,
    whiteboard_id TEXT NOT NULL,
    author TEXT NOT NULL,
    title TEXT NOT NULL,
    description TEXT NOT NULL,
    pros_json TEXT,        -- JSON array
    cons_json TEXT,        -- JSON array
    created_at INTEGER NOT NULL,
    FOREIGN KEY (whiteboard_id) REFERENCES whiteboards(id)
);

CREATE TABLE whiteboard_supporters (
    whiteboard_id TEXT NOT NULL,
    approach_id TEXT NOT NULL,
    supporter TEXT NOT NULL,
    created_at INTEGER NOT NULL,
    PRIMARY KEY (whiteboard_id, approach_id, supporter),
    FOREIGN KEY (approach_id) REFERENCES whiteboard_approaches(id)
);

CREATE TABLE whiteboard_votes (
    id TEXT PRIMARY KEY,
    whiteboard_id TEXT NOT NULL,
    approach_id TEXT NOT NULL,
    voter TEXT NOT NULL,
    position TEXT NOT NULL,  -- support|oppose|abstain|conditional
    reasoning TEXT NOT NULL,
    created_at INTEGER NOT NULL,
    UNIQUE (whiteboard_id, approach_id, voter),
    FOREIGN KEY (approach_id) REFERENCES whiteboard_approaches(id)
);
```

## Implementation Phases

### Phase 1: Core Structure
- Whiteboard data model
- SQLite storage
- Basic CRUD operations
- Status state machine

### Phase 2: MCP Integration
- MCP tools for CI interaction
- Question/Approach/Vote tools
- Status display

### Phase 3: Workflow Enforcement
- State transition validation
- Authority checks (human vs CI)
- Locking approved designs

### Phase 4: Integration
- Link to meeting room (notify on changes)
- Link to working memory (active whiteboard in focus)
- Argo integration (approved design → workflow)

## Resolved Questions (from Ami's review)

1. **Multiple whiteboards per project?**
   - **Resolution:** One whiteboard per *problem*, not per project
   - Projects can have multiple problems tracked separately
   - Sub-whiteboards can nest via `parent_id`

2. **Design format?**
   - **Resolution:** Markdown for prose, JSON for metadata
   - Human-readable, version-control friendly
   - Flexible enough for varied content

3. **Regression process?**
   - **Resolution:** CIs can request reconsideration, humans approve
   - Controlled regression with audit trail
   - Previous work preserved as historical record

## Open Questions

1. **Argo integration scope?**
   - Should approved designs auto-generate Argo workflows?
   - Or just be reference documents?

2. **History/versioning?**
   - Track all changes?
   - Or just key state transitions?

3. **Working memory integration?**
   - Active whiteboard problem/goal auto-populate shared working memory?
   - Keeps team attention aligned on current problem

## References

- Casey's Tekton Critical Thinking experiments
- Current meeting room implementation (`katra_meeting.h`)
- Argo workflow coordination patterns

---

*"The meeting room is communication. The whiteboard is consensus."*
