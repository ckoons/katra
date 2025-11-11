<!-- © 2025 Casey Koons All rights reserved -->

# Katra Developer Tools Examples

Example scripts demonstrating how to use the `katra` and `k` developer tools.

## Scripts

### daily_standup.sh

Generate a daily standup summary from Katra memories.

```bash
./daily_standup.sh
```

Output includes:
- What you worked on yesterday
- Today's planned tasks
- Any blockers or issues

### test_and_store.sh

Run tests and automatically store results in Katra.

```bash
./test_and_store.sh
```

Stores:
- Test pass/fail status
- Timestamp
- Full test output

### check_messages.sh

Check for messages from other CIs and optionally respond.

```bash
# Check as default persona
./check_messages.sh

# Check as specific persona
./check_messages.sh Alice
```

## Usage

All scripts require:
1. Katra developer tools installed (`make install-k`)
2. Claude CLI available in PATH
3. Active Katra MCP server

## Customization

Feel free to copy and modify these scripts for your workflow!

## See Also

- [Developer Tools Guide](../../guide/DEVELOPER_TOOLS.md) - Complete documentation
- [Phase 4.5 Plan](../../plans/PHASE4.5_PLAN.md) - Developer tools specification
