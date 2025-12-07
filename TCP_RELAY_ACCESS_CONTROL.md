# TCP Relay Access Control Documentation

## Overview

This document describes the TCP relay access control feature implemented in c-toxcore. This feature enables TCP relay servers to restrict connections to only whitelisted client IDs, providing a secure way to control access to the Tox network through relay servers.

## Motivation

The Tox protocol includes built-in TCP relay functionality that allows Tox clients to connect to the network when direct UDP connections aren't possible. However, the default implementation accepts connections from any client that knows the relay's public key and IP/port. This access control feature adds a whitelist mechanism to allow only authorized clients to establish TCP relay connections.

## Implementation Overview

### Core Changes

1. **TCP Server Structure**: Added access control fields to the `TCP_Server` structure
2. **Whitelist Management**: Implemented functions to manage the public key whitelist
3. **Handshake Integration**: Modified the handshake process to check whitelist membership
4. **API Extensions**: Added both internal and public API functions

### Data Structures Added

```c
// Added to TCP_Server structure:
uint8_t **whitelist_pks;          // Array of whitelisted public keys
uint16_t whitelist_count;         // Number of whitelisted keys
uint16_t whitelist_capacity;      // Max capacity of whitelist
bool access_control_enabled;      // Whether to enforce access control
```

## API Functions

### Internal API (TCP_server.h)

```c
/** Add a public key to the TCP relay whitelist */
bool tcp_server_add_to_whitelist(TCP_Server *tcp_server, const uint8_t *public_key);

/** Remove a public key from the TCP relay whitelist */
bool tcp_server_remove_from_whitelist(TCP_Server *tcp_server, const uint8_t *public_key);

/** Set whether access control is enabled */
void tcp_server_set_access_control_enabled(TCP_Server *tcp_server, bool enabled);

/** Check if a public key is whitelisted */
bool tcp_server_is_whitelisted(const TCP_Server *tcp_server, const uint8_t *public_key);
```

### Public Tox API (tox.h/tox.c)

```c
/**
 * @brief Add a public key to the TCP relay whitelist.
 * This is only relevant if the instance is acting as a TCP relay.
 *
 * @param public_key The long term public key of the client to whitelist
 *   (TOX_PUBLIC_KEY_SIZE bytes).
 * @return true on success.
 */
bool tox_add_tcp_relay_to_whitelist(Tox *tox, const uint8_t public_key[TOX_PUBLIC_KEY_SIZE]);

/**
 * @brief Remove a public key from the TCP relay whitelist.
 * This is only relevant if the instance is acting as a TCP relay.
 *
 * @param public_key The long term public key of the client to remove from whitelist
 *   (TOX_PUBLIC_KEY_SIZE bytes).
 * @return true on success.
 */
bool tox_remove_tcp_relay_from_whitelist(Tox *tox, const uint8_t public_key[TOX_PUBLIC_KEY_SIZE]);

/**
 * @brief Enable or disable access control for TCP relay connections.
 * This is only relevant if the instance is acting as a TCP relay.
 *
 * @param enabled Whether access control should be enabled.
 */
void tox_set_tcp_relay_access_control_enabled(Tox *tox, bool enabled);
```

## Usage Example

```c
#include <tox/tox.h>

// Create Tox instance with TCP server enabled
Tox_Options *options = tox_options_new(NULL);
tox_options_set_tcp_port(options, 44345); // Enable TCP server on port 44345
Tox *tox = tox_new(options, NULL);

// Enable access control
tox_set_tcp_relay_access_control_enabled(tox, true);

// Add authorized client public keys to whitelist
uint8_t authorized_client_pk[TOX_PUBLIC_KEY_SIZE];
// ... populate authorized_client_pk ...
tox_add_tcp_relay_to_whitelist(tox, authorized_client_pk);

// Only whitelisted clients can now connect to the TCP relay
```

## Security Properties

1. **Early Connection Rejection**: Non-whitelisted clients are rejected during the handshake process, before establishing any connection to the relay
2. **Backward Compatibility**: When access control is disabled (default), behavior is identical to the original implementation
3. **Memory Safety**: Proper allocation and deallocation of whitelist entries using the existing memory management system
4. **Thread Safety**: Proper locking through the existing tox_lock/tox_unlock mechanism

## Implementation Details

### Handshake Process Integration

The access control check is integrated into the TCP handshake process:

1. Receive client's initial handshake packet
2. Extract client's public key from handshake data
3. If access control is enabled, check if client's public key is in whitelist
4. If not whitelisted, reject the connection (return -1)
5. If whitelisted or access control disabled, continue with normal handshake

### Memory Management

- The whitelist array dynamically resizes when needed (doubles in capacity)
- Individual public keys are allocated separately and properly freed
- All memory is cleaned up when the TCP server is destroyed
- Memory management follows the existing c-toxcore patterns

### Performance Considerations

- Whitelist check is an O(n) operation where n is the number of whitelisted keys
- For typical use cases with small whitelists (< 1000 entries), the performance impact is minimal
- The check occurs early in the connection process, preventing resource waste on unauthorized connections

## Integration with toxvpn

After implementing this feature, toxvpn can:

1. Enable TCP relay server functionality with `tcp_port` in Tox_Options
2. Enable access control using `tox_set_tcp_relay_access_control_enabled(tox, true)`
3. Add whitelisted client public keys using `tox_add_tcp_relay_to_whitelist(tox, public_key)`
4. Allow whitelisted clients to access the Tox network through the relay while preventing unauthorized access

## Default Behavior

- Access control is **disabled by default** to maintain backward compatibility
- When disabled, the TCP relay operates as it did before this feature
- The whitelist functionality has no effect when access control is disabled

## Error Handling

- Functions return appropriate boolean values (true for success, false for failure)
- Internal array reallocation failures are properly handled
- Null pointer checks prevent crashes
- Invalid operations on empty lists are handled gracefully