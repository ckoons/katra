# TCP MCP Server Configuration

## Overview

Katra's MCP server supports both stdio (default) and TCP modes. TCP mode allows multiple Claude Code windows to connect to a single server instance, eliminating the problem of multiple server processes.

## Configuration

### Environment Variables

Configure the TCP server using `.env.katra` or `.env.katra.local` files:

```bash
# Enable TCP mode (default: false for stdio mode)
KATRA_MCP_TCP_MODE=true

# TCP port (default: 3141)
KATRA_MCP_TCP_PORT=3141

# Bind address (default: 127.0.0.1, localhost only for security)
KATRA_MCP_TCP_BIND=127.0.0.1

# Max concurrent clients (default: 32)
KATRA_MCP_TCP_MAX_CLIENTS=32

# Enable health check endpoint (default: true)
KATRA_MCP_TCP_HEALTH_CHECK=true
```

### Command-Line Flags

Command-line flags override environment variables:

```bash
# Run in TCP mode
./bin/katra_mcp_server --tcp

# TCP mode with custom port
./bin/katra_mcp_server --tcp --port 3142

# Show help
./bin/katra_mcp_server --help
```

### Priority Cascade

1. **Command-line flags** (highest priority)
2. **Environment variables** from `.env.katra.local`
3. **Environment variables** from `.env.katra`
4. **Defaults** (stdio mode, port 3141)

## Claude Code Integration

### Current Configuration (stdio mode)

File: `~/Library/Application Support/Claude/claude_desktop_config.json`

```json
{
  "mcpServers": {
    "katra": {
      "command": "/Users/cskoons/projects/github/katra/bin/katra_mcp_server",
      "args": [],
      "env": {
        "KATRA_PERSONA": "Casey-Claude-Session"
      }
    }
  }
}
```

### TCP Mode Configuration

**Option 1: Using netcat (nc)**

```json
{
  "mcpServers": {
    "katra": {
      "command": "nc",
      "args": ["localhost", "3141"],
      "alwaysAllow": true
    }
  }
}
```

**Option 2: Using socat (more features)**

```json
{
  "mcpServers": {
    "katra": {
      "command": "socat",
      "args": ["-", "TCP:localhost:3141"],
      "alwaysAllow": true
    }
  }
}
```

### Switching to TCP Mode

1. **Backup current config:**
   ```bash
   cp ~/Library/Application\ Support/Claude/claude_desktop_config.json \
      ~/Library/Application\ Support/Claude/claude_desktop_config.json.backup
   ```

2. **Start TCP server:**
   ```bash
   # Set environment variable in .env.katra.local
   echo "KATRA_MCP_TCP_MODE=true" >> .env.katra.local

   # Or start manually
   ./bin/katra_mcp_server --tcp &
   ```

3. **Update Claude Code config** to use netcat (see Option 1 above)

4. **Restart Claude Code** (fully quit and reopen)

5. **Verify connection:**
   ```bash
   # Check server is running
   ps aux | grep katra_mcp_server

   # Test health check
   curl http://localhost:3141/health
   ```

## Running the TCP Server

### Manual Start (Development)

```bash
# Start in foreground
./bin/katra_mcp_server --tcp

# Start in background
./bin/katra_mcp_server --tcp &

# Custom port
./bin/katra_mcp_server --tcp --port 3142
```

### Using Makefile

```bash
# Build and restart server
make install-mcp

# Kill old servers
make restart-mcp
```

### Health Check

The TCP server provides an HTTP health check endpoint:

```bash
# Using curl
curl http://localhost:3141/health

# Using nc
echo "GET /health HTTP/1.1\r\n\r\n" | nc localhost 3141

# Expected response
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 27

{"status":"healthy","ok":true}
```

## Features

### Multi-Tenant Support

- Up to 32 concurrent client connections
- Each connection maintains separate session state
- Persona/CI identity isolation
- Thread-per-client architecture

### Security

- Binds to 127.0.0.1 (localhost only) by default
- No network exposure unless explicitly configured
- Each client has independent authentication

### Reliability

- Graceful shutdown on SIGTERM/SIGINT
- Automatic client cleanup on disconnect
- SO_REUSEADDR for quick restart
- Health check for monitoring

## Troubleshooting

### Multiple Servers Running

```bash
# Check for running servers
ps aux | grep katra_mcp_server

# Kill all instances
make restart-mcp
# or
pkill -9 katra_mcp_server
```

### Port Already in Use

```bash
# Check what's using port 3141
lsof -i :3141

# Use a different port
export KATRA_MCP_TCP_PORT=3142
./bin/katra_mcp_server --tcp
```

### Cannot Connect from Claude Code

1. Verify server is running: `ps aux | grep katra_mcp_server`
2. Test health endpoint: `curl http://localhost:3141/health`
3. Check Claude Code config uses correct port
4. Restart Claude Code completely (Cmd+Q, not just close window)
5. Check server logs in `~/.katra/logs/`

### Performance Issues

```bash
# Increase max clients
echo "KATRA_MCP_TCP_MAX_CLIENTS=64" >> .env.katra.local

# Monitor connections
lsof -i :3141 | wc -l
```

## Development

### Testing

```bash
# Run TCP server tests
make test-tcp-server

# Test with custom config
KATRA_MCP_TCP_PORT=3199 make test-tcp-server
```

### Debugging

```bash
# Enable debug logging
export KATRA_LOG_LEVEL=DEBUG
./bin/katra_mcp_server --tcp

# Watch logs in real-time
tail -f ~/.katra/logs/katra.log
```

## Future Enhancements

- systemd service integration (Linux)
- launchd service integration (macOS)
- TLS/SSL support for encrypted connections
- Authentication/authorization per client
- Metrics and monitoring dashboard
