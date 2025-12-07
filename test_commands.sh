#!/bin/bash
# Test script for toxvpn TCP ACL commands

echo "Testing toxvpn TCP ACL commands..."

# Clean up any previous test files
rm -f /tmp/test_toxvpn.sock
mkdir -p $HOME/.toxvpn

# Start toxvpn in background
echo "Starting toxvpn with socket interface..."
sudo timeout 10s ./toxvpn -i 10.10.0.1 -l /tmp/test_toxvpn.sock -p 33445 &
TOXVPN_PID=$!

# Wait a moment for toxvpn to start
sleep 3

# Check if toxvpn is running
if ps -p $TOXVPN_PID > /dev/null; then
    echo "ToxVPN is running with PID $TOXVPN_PID"
    
    # Test help command
    echo "Testing help command..."
    if echo "help" | nc -U /tmp/test_toxvpn.sock 2>/dev/null; then
        echo "Help command successful - checking for ACL commands..."
        help_output=$(echo "help" | nc -U /tmp/test_toxvpn.sock 2>/dev/null)
        if echo "$help_output" | grep -q "tcp_acl"; then
            echo "SUCCESS: ACL commands found in help output"
            echo "$help_output" | grep "tcp_acl"
        else
            echo "ISSUE: ACL commands not found in help output"
        fi
    else
        echo "Could not connect to toxvpn socket"
    fi
    
    # Test one of the new ACL commands
    echo "Testing tcp_acl_enable command..."
    if echo "tcp_acl_enable" | nc -U /tmp/test_toxvpn.sock 2>/dev/null; then
        echo "tcp_acl_enable command executed successfully"
    else
        echo "Failed to execute tcp_acl_enable command"
    fi
    
else
    echo "ToxVPN failed to start"
fi

# Clean up
sudo kill $TOXVPN_PID 2>/dev/null || true
sleep 1
sudo pkill -f toxvpn 2>/dev/null || true
rm -f /tmp/test_toxvpn.sock

echo "Test completed"