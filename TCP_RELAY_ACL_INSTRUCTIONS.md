# TCP Relay Access Control (ACL) Usage Guide

## Overview

This guide explains how to use the TCP relay access control feature that allows you to restrict which Tox clients can connect to your TCP relay server. This feature enables you to create a private, secure relay that only authorized clients can access.

## Requirements

- Tox library with TCP relay access control feature (c-toxcore with the ACL implementation)
- A Tox instance running as a TCP relay server
- Public keys of authorized clients you want to allow

## Setup and Configuration

### 1. Enable TCP Relay Server

First, configure your Tox instance to run as a TCP relay server:

```c
#include <tox/tox.h>

// Create Tox options
Tox_Options *options = tox_options_new(NULL);

// Enable TCP server on a specific port (e.g., 443, 33445, etc.)
tox_options_set_tcp_port(options, 44345);

// Create the Tox instance
Tox *tox = tox_new(options, NULL);

// Verify the TCP server is running
Tox_Err_Get_Port error;
uint16_t port = tox_self_get_tcp_port(tox, &error);
if (port == 0) {
    printf("TCP server failed to start\n");
    // Handle error
} else {
    printf("TCP server running on port %d\n", port);
}
```

### 2. Enable Access Control

By default, access control is **disabled**. To enable it, call:

```c
// Enable access control - only whitelisted clients will be allowed
tox_set_tcp_relay_access_control_enabled(tox, true);
```

When access control is disabled (default), all clients can connect to your TCP relay.

### 3. Manage the Whitelist

#### Add a Client to the Whitelist

```c
// Define the public key of the client you want to authorize
uint8_t client_public_key[TOX_PUBLIC_KEY_SIZE];
// ... populate client_public_key with the 32-byte public key ...

// Add the client to the whitelist
bool success = tox_add_tcp_relay_to_whitelist(tox, client_public_key);
if (success) {
    printf("Client successfully added to whitelist\n");
} else {
    printf("Failed to add client to whitelist\n");
}
```

#### Remove a Client from the Whitelist

```c
// Remove a client from the whitelist
bool success = tox_remove_tcp_relay_from_whitelist(tox, client_public_key);
if (success) {
    printf("Client successfully removed from whitelist\n");
} else {
    printf("Client not found in whitelist\n");
}
```

## Complete Example

Here's a complete example showing typical usage:

```c
#include <tox/tox.h>
#include <stdio.h>
#include <string.h>

int main() {
    // Create Tox options
    Tox_Options *options = tox_options_new(NULL);
    tox_options_set_tcp_port(options, 44345);  // Enable TCP relay server on port 44345
    
    // Create Tox instance
    Tox *tox = tox_new(options, NULL);
    if (!tox) {
        printf("Failed to create Tox instance\n");
        tox_options_free(options);
        return 1;
    }
    
    // Get the public key of this relay (optional, for display purposes)
    uint8_t relay_public_key[TOX_PUBLIC_KEY_SIZE];
    tox_self_get_public_key(tox, relay_public_key);
    
    printf("TCP Relay started with public key: ");
    for (int i = 0; i < TOX_PUBLIC_KEY_SIZE; i++) {
        printf("%02x", relay_public_key[i]);
    }
    printf("\n");
    
    // Enable access control
    tox_set_tcp_relay_access_control_enabled(tox, true);
    printf("Access control has been enabled\n");
    
    // Add authorized client public keys to the whitelist
    uint8_t authorized_client1[TOX_PUBLIC_KEY_SIZE] = {0};  // Replace with actual public key
    uint8_t authorized_client2[TOX_PUBLIC_KEY_SIZE] = {0};  // Replace with actual public key
    
    // Example: populate with some test values (replace with real public keys)
    // In real code, you'd get these from your authorized clients
    for (int i = 0; i < TOX_PUBLIC_KEY_SIZE; i++) {
        authorized_client1[i] = i;        // Example pattern
        authorized_client2[i] = i + 100;  // Example pattern
    }
    
    // Add authorized clients
    if (tox_add_tcp_relay_to_whitelist(tox, authorized_client1)) {
        printf("Client 1 added to whitelist\n");
    } else {
        printf("Failed to add Client 1 to whitelist\n");
    }
    
    if (tox_add_tcp_relay_to_whitelist(tox, authorized_client2)) {
        printf("Client 2 added to whitelist\n");
    } else {
        printf("Failed to add Client 2 to whitelist\n");
    }
    
    // Now only authorized_client1 and authorized_client2 can connect to your TCP relay
    printf("TCP relay is now running with access control enabled\n");
    printf("Only whitelisted clients can connect\n");
    
    // Main loop (in real application, you would have actual tox_iterate calls here)
    // for (int i = 0; i < 1000; i++) {  // Example loop
    //     tox_iterate(tox, NULL);
    //     usleep(tox_iteration_interval(tox) * 1000);
    // }
    
    // Cleanup
    tox_kill(tox);
    tox_options_free(options);
    
    return 0;
}
```

## Important Notes

### Security Considerations

1. **Default Behavior**: Access control is disabled by default, so existing relays won't be affected until you explicitly enable it.

2. **Public Key Management**: You must obtain the 32-byte public keys of clients you want to authorize. These are the same public keys used in regular Tox communication.

3. **Connection Rejection**: Unauthorized clients attempting to connect will be rejected during the handshake process, before any connection is established.

4. **Performance**: The whitelist check has minimal performance impact since it's an early rejection during handshake.

### Operational Guidelines

1. **Dynamic Management**: You can add and remove clients from the whitelist while the server is running.

2. **Logging**: The server will log attempts by unauthorized clients (if logging is enabled), allowing you to monitor access attempts.

3. **Backup**: Consider persisting your whitelist so you don't lose it when restarting your relay server.

### Troubleshooting

- **Clients can't connect**: Verify access control is enabled and the client's public key is in the whitelist
- **Performance issues**: The whitelist lookup is O(n) where n is the number of whitelisted keys; keep the whitelist reasonably sized
- **Memory usage**: The implementation efficiently manages memory and cleans up properly

### Integration with ToxVPN

When using with ToxVPN:
1. Enable TCP port in options
2. Enable access control
3. Add the public keys of authorized VPN clients to the whitelist
4. Only authorized clients will be able to use your relay for VPN access

## API Reference

### Functions

| Function | Description |
|----------|-------------|
| `tox_set_tcp_relay_access_control_enabled(tox, enabled)` | Enable/disable access control |
| `tox_add_tcp_relay_to_whitelist(tox, public_key)` | Add client to whitelist |
| `tox_remove_tcp_relay_from_whitelist(tox, public_key)` | Remove client from whitelist |

### Behavior

- **When disabled**: All clients can connect (default behavior)
- **When enabled**: Only whitelisted clients can connect
- **Duplicate additions**: Adding the same key twice works but is unnecessary
- **Non-existent removals**: Trying to remove a non-whitelisted key returns false

This access control mechanism provides a secure way to operate private TCP relay servers for enterprise, VPN, or any other use case requiring controlled access to the Tox network.