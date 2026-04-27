# Testing Scripts

This project includes comprehensive automated testing to verify cross-language RPC compatibility.

## Test Scripts

### `test_all_combinations.sh`
Comprehensive test suite that verifies all 9 combinations using the existing client implementations:

| Server   | Client   | Status |
|----------|----------|--------|
| C        | C        | ✓      |
| C        | Go       | ✓      |
| C        | Python   | ✓      |
| Go       | C        | ✓      |
| Go       | Go       | ✓      |
| Go       | Python   | ✓      |
| Python   | C        | ✓      |
| Python   | Go       | ✓      |
| Python   | Python   | ✓      |

**Usage:**
```bash
./test_all_combinations.sh
```

**Features:**
- Builds all implementations automatically
- Uses existing client/server implementations (CClient, GoClient, PythonClient)
- Tests each combination with proper isolation
- Handles clients that run event loops (timeout is expected)
- Color-coded output (green = pass, red = fail)
- Detailed error reporting on failure
- Summary statistics

### `quick_test.sh`
Fast single-combination test for rapid verification.

**Usage:**
```bash
./quick_test.sh [SERVER_LANG] [CLIENT_LANG]
```

**Examples:**
```bash
# Test Python server with Go client
./quick_test.sh Python Go

# Test C server with C client
./quick_test.sh C C

# Default: Python server + Python client
./quick_test.sh
```

## How It Works

The test scripts:

1. **Build** all server and client implementations
2. **Start** a server in the background
3. **Run** a client (with timeout since clients run event loops)
4. **Verify** output contains successful RPC calls
5. **Report** pass/fail status

The clients run their normal main functions which:
- Make RPC calls to `add(5, 3)` and `multiply(4, 2)`
- Subscribe to event broadcasts
- Enter event loop (until timeout kills them)

A timeout exit (code 124) is treated as success since the RPC calls complete before the event loop.

## Response Format Validation

All tests verify the new error structure:
```json
{
  "result": <value or null>,
  "errorCode": <integer>,
  "errorMsg": <string or null>
}
```

**Error codes:**
- `0` - Success
- `1` - Handler error or missing method
- `2` - Invalid JSON
- `3` - Unknown method

## CI/CD Integration

The test scripts are designed to work in CI/CD pipelines:

```yaml
# Example GitHub Actions
- name: Setup Python venv
  run: |
    python3 -m venv venv
    source venv/bin/activate
    pip install -r requirements.txt

- name: Run cross-language tests
  run: ./test_all_combinations.sh
```

Exit codes:
- `0` - All tests passed
- `1` - One or more tests failed

## Python Virtual Environment

The test script automatically creates and uses a Python virtual environment if available.

**Manual setup:**
```bash
./setup_venv.sh
```

Or manually:
```bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

See [PYTHON_VENV.md](PYTHON_VENV.md) for detailed Python environment documentation.

## Debugging Failed Tests

When a test fails, check:

1. **Server logs**: `/tmp/server_output.log`
2. **Client output**: `/tmp/client_output.log`
3. **Socket cleanup**: Ensure no stale sockets exist
4. **Process cleanup**: Kill any hung server processes

Manual cleanup:
```bash
rm -f /tmp/test_*.sock
pkill -f "rpc_server|goserver"
```

## Manual Testing

You can also run any client/server combination manually:

**Terminal 1 - Start server:**
```bash
cd GoServer
./goserver
```

**Terminal 2 - Run client:**
```bash
cd CClient/build
./rpc_client
```

The client will make RPC calls, print results, then listen for broadcasts indefinitely (Ctrl+C to exit).
