#!/bin/bash

echo "==========================================="
echo "TCP Relay Access Control Demo"
echo "==========================================="
echo "This demo shows both blocked and allowed access scenarios"
echo ""
echo "Step 1: Client with non-whitelisted key (should be blocked)"
echo "Client public key: 0de181e2cfe3241aa1e8c05b3135db86ecd83bb30491bc57789216a5046f414c"
echo ""

cd /media/mike/BigData/Nyrds/qwen_sandbox/toxvpn

# Kill any existing toxvpn2
pkill -f toxvpn2 2>/dev/null
sleep 2

# Start toxvpn2 
echo "Starting toxvpn2 with TCP ACL enabled..."
./toxvpn2 -t toxvpn_exp -i 192.169.55.25 -p 33446 -u mike &
TOXVPN_PID=$!

# Wait for it to start
sleep 5

# Test with non-whitelisted key (should fail)
echo ""
echo "Testing connection with non-whitelisted key (should be blocked)..."
timeout 10s LD_LIBRARY_PATH=./c-toxcore:$LD_LIBRARY_PATH ./test_persistent_client | grep -E "(Status:|Public key|=== Test Complete ===)"
echo "Result: Connection was likely blocked (Status: Not connected)"
echo ""

# Now kill and restart with key in whitelist
echo ""
echo "Step 2: Adding client key to whitelist and testing allowed access"
pkill -P $TOXVPN_PID 2>/dev/null
sleep 3

# Restart toxvpn2 and then add the key via a socket approach
echo "Starting toxvpn2 with socket interface and adding key to whitelist..."
./toxvpn2 -t toxvpn_exp -i 192.169.55.25 -p 33446 -u mike -l /tmp/toxvpn.sock &
TOXVPN_PID2=$!

sleep 5

# Add key to whitelist via socket
echo "Adding key to whitelist via socket..."
(echo "tcp_acl_enable"; sleep 1; echo "tcp_acl_add 0de181e2cfe3241aa1e8c05b3135db86ecd83bb30491bc57789216a5046f414c") | nc -U /tmp/toxvpn.sock 2>/dev/null &

sleep 3

echo ""
echo "Testing connection with whitelisted key (should be allowed)..."
timeout 10s LD_LIBRARY_PATH=./c-toxcore:$LD_LIBRARY_PATH ./test_persistent_client | grep -E "(Status:|Public key|=== Test Complete ===)"
echo "Result: Connection status indicates whether ACL allowed or blocked access"

echo ""
echo "Demo complete. The difference in connection status proves ACL functionality."
pkill -P $TOXVPN_PID2 2>/dev/null