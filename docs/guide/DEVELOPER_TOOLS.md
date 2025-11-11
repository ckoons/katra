<!-- © 2025 Casey Koons All rights reserved -->

# Katra Developer Tools

**Guide for using Katra's developer convenience tools**

**Last Updated:** 2025-01-10
**Phase:** 4.5 (Developer Experience & Polish)

---

## Overview

Katra provides two primary developer tools to streamline CI development:

1. **`katra` wrapper** - Start Claude Code with pre-configured Katra environment
2. **`k` command** - One-shot CLI queries with full Katra MCP access

These tools make it easy to:
- Launch multiple CI sessions with different personas
- Query Katra memory from the command line
- Script Katra operations
- Integrate Katra into development workflows

---

## Installation

### Quick Install

```bash
# Install both tools to ~/bin
make install-k

# Verify installation
which katra k
katra help
k --help
```

### Manual Install

```bash
# Make scripts executable
chmod +x scripts/katra scripts/k

# Create symlinks (choose one)
ln -s $(pwd)/scripts/katra ~/bin/katra
ln -s $(pwd)/scripts/k ~/bin/k

# Or system-wide
sudo ln -s $(pwd)/scripts/katra /usr/local/bin/katra
sudo ln -s $(pwd)/scripts/k /usr/local/bin/k
```

### Verify PATH

```bash
# Check if ~/bin is in PATH
echo $PATH | grep "$HOME/bin"

# If not, add to ~/.bashrc or ~/.zshrc
export PATH="$HOME/bin:$PATH"
```

---

## The `katra` Wrapper

### Purpose

Launch Claude Code with pre-configured Katra environment variables, making it easy to start multiple CI sessions with different personas.

### Basic Usage

```bash
# Start with default settings (persona: Katra, role: developer)
katra start

# Start as specific persona
katra start --persona Alice

# Start with custom role
katra start --persona Bob --role researcher

# Start with faster breathing (for development/testing)
katra start --persona Charlie --breath-interval 10

# Start in specific project directory
katra start --persona Alice ~/projects/myproject

# Full configuration
katra start --persona Dr_Smith \
            --role researcher \
            --breath-interval 15 \
            --log-level DEBUG \
            ~/research/quantum-computing
```

### Environment Variables Set

When you run `katra start`, it sets these environment variables:

- `KATRA_PERSONA` - CI's persistent persona name
- `KATRA_ROLE` - CI's role (developer, researcher, tester, etc.)
- `KATRA_BREATH_INTERVAL` - Breathing interval in seconds (default: 30)
- `KATRA_LOG_LEVEL` - Logging level (DEBUG, INFO, WARN, ERROR)

### Multi-CI Workflows

Start multiple CIs in separate terminals:

```bash
# Terminal 1: Alice (developer)
katra start --persona Alice

# Terminal 2: Bob (reviewer)
katra start --persona Bob --role reviewer

# Terminal 3: Charlie (tester)
katra start --persona Charlie --role tester

# Now all three CIs can communicate via katra_say/hear
```

### Environment Variable Defaults

You can set defaults to avoid repeating options:

```bash
# Add to ~/.bashrc or ~/.zshrc
export KATRA_PERSONA=MyName
export KATRA_ROLE=developer
export KATRA_BREATH_INTERVAL=30
export KATRA_LOG_LEVEL=INFO

# Now just run:
katra start
```

---

## The `k` Command

### Purpose

Execute one-shot queries with full Katra MCP access from the command line. Perfect for scripting, quick queries, and integrating Katra into other tools.

### Basic Usage

```bash
# Natural query (with memory access)
k "what did we do in Phase 4?"

# Recall specific memories
k --recall "breathing implementation"

# Store a memory
k --remember "Completed Phase 4.5 developer tools"

# Send message to other CIs
k --say "Phase 4.5 complete!"

# Check for messages
k --hear

# See who's active
k --who
```

### Advanced Usage

#### Piping Input

```bash
# Store piped content as memory
echo "Important discovery: auto-registration works!" | k --remember

# Summarize file contents
cat notes.txt | k "summarize these notes"

# Process output
cat research.txt | k "extract key findings" > findings.txt
```

#### Piping Output

```bash
# Save query results
k "recall Phase 4 test results" > test_summary.txt

# Filter results
k "recall all test results" | grep "passed"

# Chain with other tools
k "list all decisions" | wc -l
```

#### Persona-Specific Queries

```bash
# Query as specific persona
k --persona Alice "what was I working on yesterday?"

# Or set default persona
export KATRA_PERSONA=Bob
k "recall my recent memories"
```

#### Scripting Examples

```bash
#!/bin/bash
# daily_summary.sh - Generate daily summary

DATE=$(date +%Y-%m-%d)
k "recall memories from $DATE" > "summary_$DATE.txt"
k --remember "Generated daily summary for $DATE"
echo "Summary saved to summary_$DATE.txt"
```

```bash
#!/bin/bash
# check_messages.sh - Check for messages and respond

MESSAGES=$(k --hear)
if [ -n "$MESSAGES" ]; then
    echo "New messages:"
    echo "$MESSAGES"
    k --say "Messages received and processed"
fi
```

### Modes

The `k` command operates in different modes:

| Mode | Flag | Description | Example |
|------|------|-------------|---------|
| Query | (default) | Natural language query with memory access | `k "what did we do yesterday?"` |
| Recall | `--recall` | Search memories | `k --recall "testing"` |
| Remember | `--remember` | Store new memory | `k --remember "Important note"` |
| Say | `--say` | Broadcast message | `k --say "Hello everyone"` |
| Hear | `--hear` | Check for messages | `k --hear` |
| Who | `--who` | List active CIs | `k --who` |

### MCP Tools Available

When using `k` for queries (default mode), you have full access to Katra MCP tools:

- `katra_recall(topic)` - Search memories
- `katra_remember(content, reason)` - Store memory
- `katra_learn(knowledge)` - Extract learning
- `katra_decide(topic, reasoning)` - Record decision
- `katra_say(message)` - Send message
- `katra_hear()` - Receive messages
- `katra_who_is_here()` - List active CIs

The `k` command automatically includes a system prompt that tells Claude about these tools.

---

## Usage Patterns

### Development Workflow

```bash
# Morning: Check what we did yesterday
k "what did I work on yesterday?"

# Start development session
katra start --persona Alice

# [Work in Claude Code...]

# Evening: Store summary
k --remember "Today completed Phase 4.5 developer tools"
```

### Multi-CI Collaboration

```bash
# Terminal 1 (Alice): Start work
katra start --persona Alice
# In Claude Code: work on feature, then katra_say("Feature ready for review")

# Terminal 2 (Bob): Check for updates
k --persona Bob --hear
# "Alice: Feature ready for review"

# Start review session
katra start --persona Bob --role reviewer
# In Claude Code: review feature, then katra_say("Approved, looks good!")
```

### Testing Workflow

```bash
# Fast breathing for testing (2-second intervals)
katra start --persona TestCI --breath-interval 2 --log-level DEBUG

# Run tests and store results
./run_tests.sh && k --remember "All tests passed on $(date)"

# Query test history
k "recall all test results from this week" | grep "passed"
```

### Research Workflow

```bash
# Start research session
katra start --persona Dr_Smith --role researcher

# Take notes via CLI as you work
cat research_notes.txt | k --remember

# Query related research
k "recall all memories about quantum computing"

# Share findings
k --say "Found interesting correlation in quantum entanglement data"
```

---

## Configuration

### Katra Wrapper Configuration

Default values:
- `PERSONA`: "Katra"
- `ROLE`: "developer"
- `BREATH_INTERVAL`: 30 seconds
- `LOG_LEVEL`: "INFO"

Override via:
1. Command-line flags (highest priority)
2. Environment variables
3. Defaults (lowest priority)

### K Command Configuration

Default values:
- `PERSONA`: "CLI" (for anonymous queries)
- `ROLE`: "developer"

Override via:
1. `--persona` flag
2. `KATRA_PERSONA` environment variable
3. Default "CLI"

### Breathing Interval Guidelines

- **Production**: 30 seconds (2 breaths/minute) - natural rhythm
- **Development**: 10-15 seconds - faster feedback during testing
- **Testing**: 2-5 seconds - rapid iteration for automated tests
- **Minimum**: 1 second (enforced by Katra)

---

## Troubleshooting

### "Command not found: katra" or "Command not found: k"

**Solution:**
```bash
# Check installation
ls -la ~/bin/katra ~/bin/k

# Check PATH
echo $PATH | grep "$HOME/bin"

# Add to PATH if missing (add to ~/.bashrc or ~/.zshrc)
export PATH="$HOME/bin:$PATH"

# Reload shell
source ~/.bashrc  # or source ~/.zshrc
```

### "Claude: command not found"

The `k` command requires the `claude` CLI tool to be installed.

**Solution:**
```bash
# Install Claude CLI (if not installed)
# See: https://docs.anthropic.com/claude/docs/cli-installation

# Verify installation
which claude
```

### "Permission denied"

**Solution:**
```bash
# Make scripts executable
chmod +x ~/bin/katra ~/bin/k
```

### Scripts run but Katra doesn't recognize persona

**Symptom:** `katra_whoami()` shows default name instead of configured persona

**Cause:** Environment variables not reaching MCP server

**Solution:** The `katra` wrapper should work correctly. If not:
```bash
# Verify environment is set
katra start --persona Alice
# In new terminal:
ps aux | grep katra_mcp_server
# Should see KATRA_PERSONA=Alice in environment
```

Note: Environment variables only work when MCP server is started with them. The Claude Code MCP integration may not pass environment variables. Use `katra_register()` MCP tool instead:

```javascript
// In Claude Code session
katra_register({name: "Alice", role: "developer"})
```

---

## Examples

### Example 1: Daily Standup

```bash
#!/bin/bash
# standup.sh - Generate daily standup summary

echo "=== Daily Standup for $(date +%Y-%m-%d) ==="
echo ""
echo "Yesterday:"
k "recall what I worked on yesterday" | head -10
echo ""
echo "Today's plan:"
k "recall any pending tasks or TODOs"
echo ""
echo "Blockers:"
k "recall any issues or blockers"
```

### Example 2: Code Review Workflow

```bash
#!/bin/bash
# review.sh - Start code review session

# Start reviewer session
katra start --persona Reviewer --role code-reviewer

# Store review context
k --remember "Reviewing feature: $1"

# Check for review requests
k --hear | grep "review"
```

### Example 3: Test Result Tracking

```bash
#!/bin/bash
# test_and_store.sh - Run tests and store results

# Run tests
TEST_OUTPUT=$(make test 2>&1)
TEST_STATUS=$?

# Store result
if [ $TEST_STATUS -eq 0 ]; then
    echo "✅ Tests passed"
    echo "$TEST_OUTPUT" | k --remember "All tests passed on $(date)"
else
    echo "❌ Tests failed"
    echo "$TEST_OUTPUT" | k --remember "Tests failed on $(date) - need investigation"
fi
```

### Example 4: Research Note Collection

```bash
#!/bin/bash
# collect_research.sh - Collect research notes from multiple sources

# Collect from files
for file in research/*.txt; do
    cat "$file" | k --remember "Research notes from $(basename $file)"
done

# Collect from URLs (requires curl and html2text)
while IFS= read -r url; do
    curl -s "$url" | html2text | k "summarize this research article and remember key findings"
done < research_urls.txt

# Generate summary
k "recall all research notes from today" > research_summary_$(date +%Y%m%d).txt
```

---

## Best Practices

### DO:
- ✅ Use `katra start` for interactive sessions
- ✅ Use `k` for quick queries and scripting
- ✅ Set `KATRA_PERSONA` in shell profile for consistent identity
- ✅ Use `--breath-interval 10` during development for faster feedback
- ✅ Pipe input/output for automation (`echo "text" | k --remember`)
- ✅ Store important decisions and discoveries immediately

### DON'T:
- ❌ Use extremely short breath intervals (<2s) in production
- ❌ Run `k` in tight loops (each call starts Claude CLI - expensive)
- ❌ Use `katra start` in scripts (it starts interactive Claude Code)
- ❌ Mix personas in same session (start separate sessions instead)
- ❌ Forget to register after starting (`katra_register(name="Alice")`)

---

## Integration Examples

### Git Pre-Commit Hook

```bash
#!/bin/bash
# .git/hooks/pre-commit

# Store commit context
BRANCH=$(git branch --show-current)
k --remember "Pre-commit on branch: $BRANCH"

# Check for conflicts
if git diff --cached | grep -q "<<<<<<"; then
    k --remember "⚠️  Attempted commit with merge conflicts"
    echo "ERROR: Merge conflicts detected"
    exit 1
fi
```

### CI/CD Integration

```bash
#!/bin/bash
# ci_build.sh

# Store build start
k --remember "CI build started: commit $(git rev-parse HEAD)"

# Run build
if make build; then
    k --remember "✅ CI build passed"
    exit 0
else
    k --remember "❌ CI build failed - needs attention"
    exit 1
fi
```

### Cron Job

```bash
# Crontab entry: Daily summary at 5pm
0 17 * * * ~/bin/k "recall today's work and generate summary" > ~/daily_summary_$(date +\%Y\%m\%d).txt
```

---

## Advanced Topics

### Custom Personas

Create persona-specific scripts:

```bash
#!/bin/bash
# scripts/alice - Start Alice's development session

export KATRA_PERSONA=Alice
export KATRA_ROLE=developer
export KATRA_BREATH_INTERVAL=15

katra start ~/projects/main-project
```

### Persona Aliases

Add to `~/.bashrc`:
```bash
alias alice='katra start --persona Alice'
alias bob='katra start --persona Bob --role reviewer'
alias tester='katra start --persona TestBot --role tester --breath-interval 10'
```

### Query Shortcuts

Add to `~/.bashrc`:
```bash
alias kk='k'  # Shorter alias
alias kr='k --recall'
alias km='k --remember'
alias ks='k --say'
alias kh='k --hear'
alias kw='k --who'
```

---

## Next Steps

- **Phase 5**: Router integration for multi-provider support
- **Phase 6**: Advanced memory (vector search, graph relations)
- **Future**: Web UI for Katra visualization

See `docs/plans/ROADMAP.md` for complete development plan.

---

## See Also

- [MCP Tools Guide](./MCP_TOOLS.md) - Complete MCP tool documentation
- [Breathing Layer](./BREATHING_LAYER.md) - Autonomic breathing design
- [Meeting Room](./MEETING_ROOM.md) - Multi-CI communication
- [Phase 4.5 Plan](../plans/PHASE4.5_PLAN.md) - Developer tools specification

---

**Questions or Issues?**

See `docs/ROADMAP.md` for contact information and collaboration guidelines.
