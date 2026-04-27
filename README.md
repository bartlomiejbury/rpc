# RPC Client and Server

A multi-language RPC (Remote Procedure Call) implementation using ZeroMQ for inter-process communication. This project demonstrates service discovery and RPC patterns across C, Go, and Python.

## Overview

This project provides RPC server and client implementations in three languages:
- **C** - High-performance implementation using ZeroMQ and yyjson
- **Go** - Idiomatic Go implementation with goroutines
- **Python** - Simple and readable Python implementation

All implementations use:
- **ZeroMQ** for message transport (REP/REQ pattern for RPC, PUB/SUB for broadcasts)
- **IPC sockets** (`/tmp/*.sock`) for local communication
- **JSON** for message serialization

## Features

- ✅ Cross-language RPC communication
- ✅ Request/Reply pattern for synchronous calls
- ✅ Publish/Subscribe pattern for event broadcasting
- ✅ Service registration and discovery
- ✅ Example RPC methods (add, multiply)
- ✅ Real-time event publishing (time updates)

## Project Structure

```
ServiceDiscoveryProject/
├── CClient/          # C RPC client implementation
├── CServer/          # C RPC server implementation
├── GoClient/         # Go RPC client implementation
├── GoServer/         # Go RPC server implementation
├── PythonClient/     # Python RPC client implementation
└── PythonServer/     # Python RPC server implementation
```

## Prerequisites

### For C Implementation
- CMake 3.10+
- GCC or Clang
- ZeroMQ development libraries (`libzmq`)
- libevent development libraries (`libevent`)
- yyjson library (for JSON parsing)

### For Go Implementation
- Go 1.16+
- ZeroMQ Go bindings (`github.com/pebbe/zmq4`)

### For Python Implementation
- Python 3.7+
- PyZMQ library (`pyzmq`)

**Recommended: Use virtual environment**
```bash
python3 -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
pip install -r requirements.txt
```

## Installation

### C Dependencies

**Install yyjson:**

```bash
# yyjson is typically included via CMake or as a submodule
# If you need to install it separately:
sudo apt-get install libyyjson-dev  # if available in your distro

# Or build from source:
git clone https://github.com/ibireme/yyjson.git
cd yyjson
mkdir build && cd build
cmake ..
make
sudo make install
```

**Note:** Some C implementations may include yyjson as a header-only library or git submodule, in which case no separate installation is needed.

**Install ZeroMQ:**

```bash
sudo apt-get update
sudo apt-get install libzmq3-dev libevent-dev
```

### Python Dependencies

**Using virtual environment (recommended):**

```bash
# Create virtual environment
python3 -m venv venv

# Activate virtual environment
source venv/bin/activate  # On Windows: venv\Scripts\activate

# Install dependencies
pip install -r requirements.txt

# Verify installation
python3 -c "import zmq; print(f'PyZMQ version: {zmq.zmq_version()}')"
```

**Alternative - system-wide installation:**

```bash
# Using pip
pip install pyzmq
```

To deactivate the virtual environment when done:
```bash
deactivate
```

### Go Dependencies

The Go implementations use Go modules for dependency management. Dependencies will be automatically downloaded when you build:

```bash
cd GoServer  # or GoClient
go mod download
```

Or let `go build` handle it automatically:
```bash
cd GoServer
go build  # This will download dependencies if needed
```

The main dependency is:
- `github.com/pebbe/zmq4` v1.4.0 - ZeroMQ bindings for Go


## Quick Start

The fastest way to get started is with Python (using virtual environment):

**Terminal 1 - Start the server:**
```bash
# Setup Python environment (first time only)
./setup_venv.sh

# Activate virtual environment
source venv/bin/activate

cd PythonServer
python rpc_server.py
```

**Terminal 2 - Run the client:**
```bash
# Activate virtual environment
source venv/bin/activate

cd PythonClient
python rpc_client.py
```

You should see RPC call results and periodic time updates from the server!

For Go or C implementations, see the detailed build instructions below.

For more details on Python virtual environment, see [PYTHON_VENV.md](PYTHON_VENV.md).

## Building and Running

### C Implementation

**Server:**
```bash
cd CServer/build
cmake ..
make
./rpc_server
```

**Client:**
```bash
cd CClient/build
cmake ..
make
./rpc_client
```

### Go Implementation

**Server:**
```bash
cd GoServer
go build
./goserver
```

**Client:**
```bash
cd GoClient
go build
./goclient
```

### Python Implementation

**Server:**
```bash
cd PythonServer
python rpc_server.py
```

**Client:**
```bash
cd PythonClient
python rpc_client.py
```

## Usage Example

1. **Start a server** (any language):
   ```bash
   cd GoServer
   ./goserver
   ```

2. **Run a client** (any language):
   ```bash
   cd PythonClient
   python rpc_client.py
   ```

The client can call RPC methods on the server and subscribe to broadcasts, regardless of the language either is written in!

## RPC Methods

The example implementations include:

- **add(a, b)** - Returns the sum of two numbers
- **multiply(a, b)** - Returns the product of two numbers

## Event Broadcasting

Servers periodically publish events on the PUB socket:
- **time** - Current timestamp (published every 2 seconds)

Clients can subscribe to these events to receive real-time updates.

## API Reference

### Service Naming Convention

All implementations use a service name pattern where a service named `"test"` creates:
- RPC socket: `ipc:///tmp/test_rpc.sock`
- PUB socket: `ipc:///tmp/test_pub.sock`

### C API

**Server:**
```c
RPCServer* rpc_server_new(const char *name);
void rpc_server_register(RPCServer *s, const char *name, rpc_handler_t handler, void *user_data);
int rpc_server_start(RPCServer *s);
void rpc_server_publish(RPCServer *s, const char *topic, const char *json_msg);
void rpc_server_stop(RPCServer *s);
void rpc_server_free(RPCServer *s);
```

**Client:**
```c
RPCPubSubClient* rpc_client_new(const char *name);
int rpc_subscribe(RPCPubSubClient *c, const char *topic,
                  void (*cb)(const char*, const char*, void*), void *user_data);
int rpc_call(RPCPubSubClient *c, int timeout_ms, char *resp, size_t resp_size,
             char *err, size_t err_size, const char *method, const char *fmt, ...);
void rpc_client_free(RPCPubSubClient *c);
```

### Go API

**Server:**
```go
server := NewRPCServer(name string)
server.Register(method string, handler func([]interface{}) (interface{}, error))
server.Start() error
server.Publish(topic string, data interface{})
server.Stop()
```

**Client:**
```go
client, err := NewRPCPubSubClient(name string)
result, err := client.Call(timeout_ms int, method string, params ...interface{})
err := client.Subscribe(topic string, callback func(string, string))
err := client.Unsubscribe(topic string)
client.Stop()
```

### Python API

**Server:**
```python
server = RPCServer(name: str)
server.register(name: str, func: Callable[[List[Any]], Any])
server.start()
server.publish(topic: str, data: Dict[str, Any])
server.stop()
```

**Client:**
```python
client = RPCPubSubClient(name: str)
result = client.call(method: str, *params, timeout_ms: int = 3000)
# Alternative: client.method_name(*params)  # Dynamic method calling
client.subscribe(topic: str, callback: Callable[[str, str], None])
client.unsubscribe(topic: str)
client.stop()
```

## Architecture

### RPC Communication
- **Pattern**: REQ/REP (Request/Reply)
- **Transport**: IPC sockets (`ipc:///tmp/<service>_rpc.sock`)
- **Format**: JSON messages
  - Request: `{"method": "add", "params": [5, 3]}`
  - Success Response: `{"result": 8, "error": null}`
  - Error Response: `{"result": null, "error": "error message"}`
- **Timeout**: Configurable per-call (default: 3000ms)
- **Thread Safety**: Each implementation handles concurrent RPC calls safely

### Event Broadcasting
- **Pattern**: PUB/SUB (Publish/Subscribe)
- **Transport**: IPC sockets (`ipc:///tmp/<service>_pub.sock`)
- **Format**: Topic-based JSON messages
  - Topic: String identifier (e.g., "time", "weather")
  - Payload: JSON string
- **Delivery**: Fire-and-forget, no acknowledgment
- **Subscription**: Clients can subscribe/unsubscribe at runtime

### Implementation Details

**C Implementation:**
- Uses `libevent` for non-blocking event loop integration
- `yyjson` for high-performance JSON parsing
- Separate threads for RPC handling and event publishing
- Manual memory management with proper cleanup

**Go Implementation:**
- Goroutines for concurrent RPC handling
- Channels for communication between components
- Built-in JSON marshaling/unmarshaling
- Graceful shutdown with signal handling

**Python Implementation:**
- Threading for background subscription listener
- Context manager support for resource cleanup
- Dynamic method calling via `__getattr__`
- Simple and readable implementation

## Cross-Language Compatibility

All implementations use the same wire protocol, allowing:
- Go client → Python server
- C client → Go server
- Python client → C server
- Any combination!

## Development

### Adding New RPC Methods

Register new methods in the server implementation:

**C:**
```c
yyjson_mut_val* my_handler(yyjson_mut_doc *doc, yyjson_val *params,
                            void *user_data, char *err_buf, size_t err_size) {
    // Parse parameters from JSON array
    if (!yyjson_is_arr(params)) {
        snprintf(err_buf, err_size, "params must be an array");
        return NULL;
    }

    // Process and return result
    int result = /* your calculation */;
    return yyjson_mut_int(doc, result);
}

// Register the handler
rpc_server_register(server, "myMethod", my_handler, NULL);
```

**Go:**
```go
server.Register("myMethod", func(params []interface{}) (interface{}, error) {
    // Type assertions for parameters
    arg1 := int(params[0].(float64))
    arg2 := params[1].(string)

    // Process and return result
    return result, nil
})
```

**Python:**
```python
def my_handler(params):
    arg1, arg2 = params[0], params[1]
    # Process and return result
    return result

server.register("myMethod", my_handler)
```

### Publishing Custom Events

**C:**
```c
rpc_server_publish(server, "custom_event", "{\"data\": \"value\"}");
```

**Go:**
```go
server.Publish("custom_event", map[string]interface{}{
    "data": "value",
})
```

**Python:**
```python
server.publish("custom_event", {"data": "value"})
```

### Complete Example

Here's a complete example of creating a temperature service:

**Python Server (temp_server.py):**
```python
from PythonServer.rpc_server import RPCServer
import time
import random

server = RPCServer("temperature")

def celsius_to_fahrenheit(params):
    celsius = params[0]
    return (celsius * 9/5) + 32

def fahrenheit_to_celsius(params):
    fahrenheit = params[0]
    return (fahrenheit - 32) * 5/9

server.register("c_to_f", celsius_to_fahrenheit)
server.register("f_to_c", fahrenheit_to_celsius)
server.start()

# Publish temperature readings every 5 seconds
try:
    while True:
        temp = round(random.uniform(15.0, 30.0), 1)
        server.publish("temperature", {"celsius": temp, "fahrenheit": round((temp * 9/5) + 32, 1)})
        time.sleep(5)
except KeyboardInterrupt:
    server.stop()
```

**Go Client (temp_client.go):**
```go
package main

import (
    "fmt"
    "log"
)

func main() {
    client, err := NewRPCPubSubClient("temperature")
    if err != nil {
        log.Fatal(err)
    }
    defer client.Stop()

    // Subscribe to temperature updates
    client.Subscribe("temperature", func(topic, payload string) {
        fmt.Printf("Temperature update: %s\n", payload)
    })

    // Convert temperature
    result, _ := client.Call(3000, "c_to_f", 25.0)
    fmt.Printf("25°C = %.1f°F\n", result)

    // Keep running to receive updates
    select {}
}
```

## License

See [LICENSE.txt](LICENSE.txt) for details.

## Contributing

Contributions are welcome! Feel free to:
- Add implementations in other languages
- Enhance existing implementations
- Add more example RPC methods
- Improve documentation

## Testing

### Test Results

The automated test suite (`./test_all_combinations.sh`) has been successfully tested:

| Server | Client | Status | Notes |
|--------|--------|--------|-------|
| C | Go | ✓ PASSED | Full compatibility |
| Go | Go | ✓ PASSED | Full compatibility |

**Note:** Python tests require dependencies to be installed:
```bash
# Recommended: Use virtual environment
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt

# Then run tests
./test_all_combinations.sh
```

After installing Python dependencies, all 9 combinations (C, Go, Python × C, Go, Python) should pass.

### Automated Cross-Language Testing

Test all 9 combinations of servers and clients (C, Go, Python × C, Go, Python):

```bash
./test_all_combinations.sh
```

This script will:
1. Build all server and client implementations
2. Test each combination systematically
3. Report pass/fail for each test
4. Display a summary of results

Example output:
```
========================================
RPC Cross-Language Compatibility Test
========================================

[1] Testing: C Server ↔ C Client
✓ PASSED
[2] Testing: C Server ↔ Go Client
✓ PASSED
[3] Testing: C Server ↔ Python Client
✓ PASSED
...
========================================
Test Summary
========================================
Total tests:  9
Passed:       9
Failed:       0

✓ All tests passed!
```

### Quick Test

Test a specific server-client combination:

```bash
# Usage: ./quick_test.sh [SERVER_LANG] [CLIENT_LANG]

# Test Python server with Go client
./quick_test.sh Python Go

# Test C server with Python client
./quick_test.sh C Python

# Test Go server with C client
./quick_test.sh Go C
```

The test scripts use the existing client implementations (CClient, GoClient, PythonClient), which make RPC calls and then enter an event loop. The tests verify successful RPC communication before timing out.

See [TESTING.md](TESTING.md) for comprehensive testing documentation.

## Troubleshooting

### Socket Already in Use
If you get "Address already in use" errors:
```bash
rm /tmp/rpc.sock /tmp/pub.sock
```

### ZeroMQ Installation Issues
Make sure ZeroMQ is properly installed and linked. Check:
```bash
pkg-config --cflags --libs libzmq
```

### Cross-Language Communication Issues
Ensure all implementations use the same:
- Socket addresses (IPC paths)
- Message format (JSON structure)
- ZeroMQ socket types (REQ/REP, PUB/SUB)
