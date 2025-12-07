#!/bin/bash

# Script to run toxvpn2 using toxvpn_exp interface with a single bootstrap node for TCP relay only
# This script configures toxvpn2 to use only one specific node as TCP bootstrap
# Usage: ./run_toxvpn2_single_node.sh [vpn_ip] [bind_port] [user] [bootstrap_ip:port:public_key]

set -e  # Exit on any error

echo "ToxVPN2 Single Bootstrap Node Runner"
echo "====================================="

# Function to display usage
usage() {
    echo "Usage: $0 [VPN_IP] [BIND_PORT] [USER] [BOOTSTRAP_IP:PORT:PUBLIC_KEY]"
    echo ""
    echo "Parameters:"
    echo "  VPN_IP           - VPN IP address for this node (default: 192.169.55.25)"
    echo "  BIND_PORT        - Port for toxvpn2 to bind to (default: 33446)"
    echo "  USER             - User to switch to after root ops (default: current user)"
    echo "  BOOTSTRAP_IP:PORT:PUBLIC_KEY - Bootstrap node details (default: 144.217.167.73:33445:7E5668E0EE09E19F320AD47902419331FFEE147BB3606769CFBE921A2A2FD34C)"
    echo ""
    echo "Example: $0 192.169.55.100 33447 myuser '144.217.167.73:33445:7E5668E0EE09E19F320AD47902419331FFEE147BB3606769CFBE921A2A2FD34C'"
    exit 1
}

# Parse command line arguments
VPN_IP=${1:-"192.169.55.25"}
BIND_PORT=${2:-"33446"}
RUN_USER=${3:-$(whoami)}
BOOTSTRAP_INFO=${4:-"144.217.167.73:33445:7E5668E0EE09E19F320AD47902419331FFEE147BB3606769CFBE921A2A2FD34C"}

# Parse bootstrap information
IFS=':' read -r BOOTSTRAP_IP BOOTSTRAP_PORT BOOTSTRAP_KEY <<< "$BOOTSTRAP_INFO"

# Validate that we have all three parts
if [ -z "$BOOTSTRAP_IP" ] || [ -z "$BOOTSTRAP_PORT" ] || [ -z "$BOOTSTRAP_KEY" ]; then
    echo "Error: Invalid bootstrap format. Use: IP:PORT:PUBLIC_KEY"
    usage
fi

# Create directory for toxvpn configs if it doesn't exist
mkdir -p .toxvpn

# Create a minimal bootstrap.json with only the specified bootstrap node for TCP bootstrap
cat > bootstrap.json << EOF
{
  "nodes": [
    {
      "ipv4": "$BOOTSTRAP_IP",
      "ipv6": "-",
      "port": $BOOTSTRAP_PORT,
      "tcp_ports": [$BOOTSTRAP_PORT],
      "public_key": "$BOOTSTRAP_KEY",
      "maintainer": "User Specified",
      "location": "User Specified",
      "status_tcp": true,
      "status_udp": true
    }
  ]
}
EOF

echo "Created single node bootstrap configuration for TCP bootstrap"
echo "Bootstrap node: $BOOTSTRAP_IP:$BOOTSTRAP_PORT"

echo "Starting toxvpn2 with:"
echo "  - VPN IP: $VPN_IP"
echo "  - Bind Port: $BIND_PORT"
echo "  - User: $RUN_USER"
echo "  - Bootstrap: $BOOTSTRAP_IP:$BOOTSTRAP_PORT"

# Kill any existing toxvpn2 processes first
echo "Stopping any existing toxvpn2 processes..."
pkill -f toxvpn2 2>/dev/null || true
sleep 2

echo "Starting toxvpn2 with toxvpn_exp interface and custom TCP bootstrap node..."
echo "Command: ./toxvpn2 -t toxvpn_exp -i $VPN_IP -p $BIND_PORT -u $RUN_USER -c .toxvpn/config.json"

# Run toxvpn2 with the specified parameters
# -t specifies the TUN interface name as "toxvpn_exp"
# -i sets the VPN IP address
# -p sets the bind port
# -u specifies the user to switch to after root operations
# -c specifies the config file location
./toxvpn2 -t toxvpn_exp -i $VPN_IP -p $BIND_PORT -u $RUN_USER -c .toxvpn/config.json

if [ $? -eq 0 ]; then
    echo "toxvpn2 started successfully with custom TCP bootstrap node"
else
    echo "Failed to start toxvpn2"
    exit 1
fi

echo "ToxVPN2 is now running with:"
echo "  - TUN interface: toxvpn_exp"
echo "  - VPN IP: $VPN_IP"
echo "  - Bind port: $BIND_PORT"
echo "  - Config file: .toxvpn/config.json"
echo "  - Bootstrap node: $BOOTSTRAP_IP:$BOOTSTRAP_PORT (TCP enabled)"