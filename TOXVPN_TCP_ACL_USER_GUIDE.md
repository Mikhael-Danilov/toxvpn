# ToxVPN TCP Relay Access Control (ACL) User Guide

## Overview

This guide explains how to use the TCP relay access control feature in ToxVPN to restrict which clients can connect to your VPN relay. This allows you to create a private, secure VPN service where only authorized clients can connect.

## Prerequisites

- ToxVPN compiled with TCP relay ACL support
- A running ToxVPN instance configured as a TCP relay
- Public keys of authorized VPN clients

## Setup as TCP Relay with ACL

### 1. Start ToxVPN as a TCP Relay Server

Start your ToxVPN instance with a TCP port enabled:

```bash
# Load the TUN module (Linux)
sudo modprobe tun

# Start ToxVPN with an IP address for the VPN interface
sudo ./toxvpn -i 10.10.0.1 -p 44345  # Use port 44345 for TCP relay
```

### 2. Use Control Commands to Configure ACL

Once ToxVPN is running, you can interact with it through the control interface. Type commands at the ToxVPN prompt:

```
help
```

You should see the new ACL commands in the help output.

## Configuration Methods

All TCP relay access control commands can be accessed through either:
- The interactive console when ToxVPN runs without special flags
- A Unix socket interface when using the `-l <socket_path>` flag

## Running ToxVPN without sudo using pre-created TUN interface

You can run ToxVPN without full root privileges by pre-creating the TUN interface and setting proper permissions.

### Steps to run without sudo:

1. **Pre-create the TUN interface with permissions**:
```bash
# Create the TUN interface (requires root) - use a different name if toxvpn0 exists
sudo ip tuntap add dev toxvpn_exp mode tun user $USER

# Set interface up and configure (requires root)
sudo ip link set toxvpn_exp up
sudo ip addr add 192.169.55.25/24 dev toxvpn_exp
sudo sysctl -w net.ipv4.ip_forward=1  # if needed for routing
```

2. **Run ToxVPN with the pre-created interface**:
```bash
# Now run toxvpn without sudo using the new -t flag
./toxvpn -t toxvpn_exp -i 192.169.55.25 -p 33445
```

3. **Configure ACL using the console interface**:
```
tcp_acl_enable
tcp_acl_add a748b3864b83610e60215e58729e31ba04b56823b5d8d5430a6f3b573d12abcd
```

### New Command-line Options:
- `-t <device>`: Use an existing TUN device instead of creating one (e.g., `-t toxvpn_exp`)
- New help shows: `-t <dev>    use existing TUN device (e.g., tun0)`

### Benefits of this approach:
- Reduces attack surface by running with fewer privileges
- The TUN interface management is separated from the application
- Can be combined with ACL functionality for private VPNs

### Method 1: Runtime Commands (Interactive Console)

Run ToxVPN interactively and configure ACL using runtime commands:

1. Start ToxVPN and keep it in the foreground:
```bash
sudo modprobe tun  # Load TUN module first
sudo ./toxvpn -i 10.10.0.1 -p 44345  # Start with TCP port
```

2. At the ToxVPN prompt, use these commands:
```
# Enable TCP relay access control
tcp_acl_enable

# Add authorized clients to the whitelist (use their 64-character hex public keys)
tcp_acl_add a748b3864b83610e60215e58729e31ba04b56823b5d8d5430a6f3b573d12abcd

# Add multiple authorized clients
tcp_acl_add bcd9e2f3748a9b1c2d3e4f5a6b7c8d9e0f1a2b3c4d5e6f7a8b9c0d1e2f3abcd1
tcp_acl_add cdef1234567890abcd1234567890abcd1234567890abcd1234567890abcd2

# Remove a client from the whitelist (if needed)
tcp_acl_remove a748b3864b83610e60215e58729e31ba04b56823b5d8d5430a6f3b573d12abcd

# Disable access control (all clients allowed again)
tcp_acl_disable

# Check available ACL commands
help
```

### Method 2: Runtime Commands (Socket Interface)

Run ToxVPN with a Unix socket to control it remotely:

```bash
# Start ToxVPN with socket interface
sudo ./toxvpn -i 10.10.0.1 -l /tmp/toxvpn.sock -p 44345 &

# Control the instance using the socket
echo "tcp_acl_enable" | nc -U /tmp/toxvpn.sock
echo "tcp_acl_add a748b3864b83610e60215e58729e31ba04b56823b5d8d5430a6f3b573d12abcd" | nc -U /tmp/toxvpn.sock
echo "help" | nc -U /tmp/toxvpn.sock
```

### Method 2: Startup Configuration

You can also build the ACL functionality into your startup scripts:

```bash
#!/bin/bash
# start_toxvpn_with_acl.sh

# Start ToxVPN in the background
sudo ./toxvpn -i 10.10.0.1 -p 44345 &

# Wait a moment for it to start
sleep 3

# Connect and configure ACL automatically
echo "tcp_acl_enable" | nc -U /path/to/unix/socket  # If using unix socket
# OR interact manually through the console
```

## Complete Usage Example

Here's a complete example of setting up a private VPN relay:

```bash
# 1. Load TUN module
sudo modprobe tun

# 2. Start ToxVPN
sudo ./toxvpn -i 10.10.0.1

# 3. At the ToxVPN prompt, get your relay's public key from the status
status
# Output: "my id is <64-char hex key> and IP is 10.10.0.1"
# The first 64 characters of your ID is your public key

# 4. Enable ACL to restrict connections
tcp_acl_enable

# 5. Add authorized clients (get their public keys from them)
tcp_acl_add a748b3864b83610e60215e58729e31ba04b56823b5d8d5430a6f3b573d12abcd
tcp_acl_add bcd9e2f3748a9b1c2d3e4f5a6b7c8d9e0f1a2b3c4d5e6f7a8b9c0d1e2f3abcd1

# 6. Your VPN relay is now private - only whitelisted clients can connect
```

## Command Reference

### ACL Commands

| Command | Description |
|---------|-------------|
| `tcp_acl_enable` | Enable TCP relay access control (only whitelisted clients allowed) |
| `tcp_acl_disable` | Disable TCP relay access control (all clients allowed) |
| `tcp_acl_add <public_key>` | Add a client's 64-character public key to the whitelist |
| `tcp_acl_remove <public_key>` | Remove a client's public key from the whitelist |
| `tcp_acl_status` | Show current ACL status |
| `help` | Show all available commands including ACL commands |

### Getting Client Public Keys

To allow clients to connect to your private VPN:

1. Have each client start their ToxVPN instance
2. Each client runs the `status` command to get their ID
3. The first 64 characters (before the nospam and checksum) is their public key
4. You add this public key to your relay's whitelist using `tcp_acl_add`

## Important Notes

### Security Considerations

- **Public Keys**: Each authorized client needs to provide you with their 64-character hex public key
- **Connection Verification**: Only clients with whitelisted public keys can connect to your TCP relay
- **Early Rejection**: Unauthorized clients are rejected during the handshake, preventing resource usage

### Operational Notes

- **Persistence**: ACL configuration is not automatically saved between restarts (you need to reconfigure after restart)
- **Default Behavior**: When disabled, all clients can connect (standard behavior)
- **Performance**: Minimal performance impact - checks happen during initial connection handshake

### Client Configuration

Authorized clients configure their ToxVPN normally, but will only be able to connect if:
1. They know your relay's IP and port
2. Your relay's public key 
3. Their public key is in your whitelist

## Troubleshooting

**Q: Clients can't connect to my private relay**
A: Verify:
1. You've enabled ACL with `tcp_acl_enable`
2. The client's public key is in your whitelist with `tcp_acl_add`
3. The client is using the correct relay information

**Q: How do I find a client's public key?**
A: Clients run `status` in their ToxVPN console - the first 64 hex characters of their ID is their public key

**Q: Does this affect UDP connections?**
A: This ACL only affects TCP relay connections. UDP connections follow Tox's normal behavior.

## Real-World Scenario

```
# VPN Admin setup:
tcp_acl_enable
tcp_acl_add a748b3864b83610e60215e58729e31ba04b56823b5d8d5430a6f3b573d12abcd  # Client 1
tcp_acl_add bcd9e2f3748a9b1c2d3e4f5a6b7c8d9e0f1a2b3c4d5e6f7a8b9c0d1e2f3abcd1  # Client 2

# Client 1 connects normally using your relay's IP, port, and public key
# Client 2 connects normally using your relay's IP, port, and public key
# Unauthorized clients attempting to connect will be rejected
```

This access control mechanism allows you to create a private VPN service where only your authorized clients can access the Tox network through your relay.