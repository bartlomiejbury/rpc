#!/bin/bash

# Test all server/client combinations and compare outputs

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

PASSED=0
FAILED=0

# Function to build all components
build_all() {
    echo "========================================"
    echo "Building All Components"
    echo "========================================"
    echo ""
    
    # Build C Server
    echo "Building C Server..."
    cd CServer/build
    cmake .. > /dev/null 2>&1 && make > /dev/null 2>&1
    if [[ $? -eq 0 ]]; then
        echo -e "${GREEN}✓ C Server built successfully${NC}"
    else
        echo -e "${RED}✗ C Server build failed${NC}"
        cd ../..
        return 1
    fi
    cd ../..
    
    # Build C Client
    echo "Building C Client..."
    cd CClient/build
    cmake .. > /dev/null 2>&1 && make > /dev/null 2>&1
    if [[ $? -eq 0 ]]; then
        echo -e "${GREEN}✓ C Client built successfully${NC}"
    else
        echo -e "${RED}✗ C Client build failed${NC}"
        cd ../..
        return 1
    fi
    cd ../..
    
    # Build Go Server
    echo "Building Go Server..."
    cd GoServer
    go build -o goserver . > /dev/null 2>&1
    if [[ $? -eq 0 ]]; then
        echo -e "${GREEN}✓ Go Server built successfully${NC}"
    else
        echo -e "${RED}✗ Go Server build failed${NC}"
        cd ..
        return 1
    fi
    cd ..
    
    # Build Go Client
    echo "Building Go Client..."
    cd GoClient
    go build -o goclient . > /dev/null 2>&1
    if [[ $? -eq 0 ]]; then
        echo -e "${GREEN}✓ Go Client built successfully${NC}"
    else
        echo -e "${RED}✗ Go Client build failed${NC}"
        cd ..
        return 1
    fi
    cd ..
    
    # Setup Python environment
    echo "Setting up Python environment..."
    if [[ -d "venv" ]]; then
        echo -e "${GREEN}✓ Python virtual environment ready${NC}"
    else
        echo -e "${YELLOW}⚠ Python virtual environment not found${NC}"
    fi
    
    echo ""
    return 0
}

# Function to test a server with all clients
test_server() {
    local SERVER_NAME=$1
    local SERVER_CMD=$2
    local SERVER_PROCESS=$3
    
    echo ""
    echo "========================================"
    echo "Testing $SERVER_NAME with All Clients"
    echo "========================================"
    echo ""
    
    # Cleanup and start server
    pkill -9 -f "$SERVER_PROCESS" 2>/dev/null
    rm -f /tmp/test_*.sock /tmp/client_*.log
    
    if [[ "$SERVER_NAME" == "Python Server" ]]; then
        source venv/bin/activate
    fi
    
    eval "$SERVER_CMD > /dev/null 2>&1 &"
    SERVER_PID=$!
    sleep 2
    
    # Test C Client
    echo "Testing C Client..."
    stdbuf -oL timeout 2 CClient/build/rpc_client > /tmp/client_c.log 2>&1
    sleep 0.5
    
    # Test Go Client
    echo "Testing Go Client..."
    stdbuf -oL timeout 2 GoClient/goclient > /tmp/client_go.log 2>&1
    sleep 0.5
    
    # Test Python Client
    echo "Testing Python Client..."
    if [[ "$SERVER_NAME" != "Python Server" ]]; then
        source venv/bin/activate
    fi
    stdbuf -oL timeout 2 python3 -u PythonClient/rpc_client.py > /tmp/client_py.log 2>&1
    sleep 0.5
    
    # Cleanup server
    kill $SERVER_PID 2>/dev/null
    wait $SERVER_PID 2>/dev/null
    pkill -9 -f "$SERVER_PROCESS" 2>/dev/null
    
    # Extract RPC results from log files
    echo ""
    echo "Output Comparison:"
    echo "------------------"
    
    echo "C Client:"
    grep -E "(add|multiply)\(" /tmp/client_c.log 2>/dev/null || echo "(no output)"
    echo ""
    
    echo "Go Client:"
    grep -E "(add|multiply)\(" /tmp/client_go.log 2>/dev/null || echo "(no output)"
    echo ""
    
    echo "Python Client:"
    grep -E "(add|multiply)\(" /tmp/client_py.log 2>/dev/null || echo "(no output)"
    echo ""
    
    # Extract and compare
    C_OUT=$(grep -E "(add|multiply)\(" /tmp/client_c.log 2>/dev/null | sort)
    GO_OUT=$(grep -E "(add|multiply)\(" /tmp/client_go.log 2>/dev/null | sort)
    PY_OUT=$(grep -E "(add|multiply)\(" /tmp/client_py.log 2>/dev/null | sort)
    
    if [[ "$C_OUT" == "$GO_OUT" ]] && [[ "$GO_OUT" == "$PY_OUT" ]] && [[ -n "$C_OUT" ]]; then
        echo -e "${GREEN}✓ All clients produce identical output with $SERVER_NAME!${NC}"
        ((PASSED++))
        return 0
    else
        echo -e "${RED}✗ Outputs differ between clients with $SERVER_NAME${NC}"
        ((FAILED++))
        return 1
    fi
}

# Main test execution
echo "========================================"
echo "Testing All Server/Client Combinations"
echo "========================================"

# Build all components first
build_all
if [[ $? -ne 0 ]]; then
    echo -e "${RED}✗ Build failed, cannot proceed with tests${NC}"
    exit 1
fi

# Test 1: C Server
test_server "C Server" "CServer/build/rpc_server" "rpc_server"

# Test 2: Go Server
test_server "Go Server" "GoServer/goserver" "goserver"

# Test 3: Python Server
test_server "Python Server" "python3 PythonServer/rpc_server.py" "PythonServer/rpc_server.py"

# Final summary
echo ""
echo "========================================"
echo "Final Summary"
echo "========================================"
echo -e "Passed: ${GREEN}$PASSED${NC}"
echo -e "Failed: ${RED}$FAILED${NC}"
echo ""

if [[ $FAILED -eq 0 ]]; then
    echo -e "${GREEN}✓ All server/client combinations work correctly!${NC}"
    exit 0
else
    echo -e "${RED}✗ Some combinations failed${NC}"
    exit 1
fi
