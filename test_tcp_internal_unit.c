/*
 * Unit tests for internal TCP server functions
 * These tests require access to internal functions in TCP_server.c
 */

#include "c-toxcore/toxcore/tox.h"
#include "c-toxcore/toxcore/Messenger.h"  // To access internal structures
#include "c-toxcore/toxcore/TCP_server.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

// Helper function to generate a deterministic public key
void generate_test_key(uint8_t *key, int seed) {
    for (int i = 0; i < 32; i++) {  // Using 32 since that's the crypto public key size
        key[i] = (seed + i) % 256;
    }
}

// Test the public API that exercises internal functions
bool test_public_api_for_internal_functions() {
    printf("Testing public API that exercises internal functions...\n");

    // Create Tox instance to test the public API
    Tox_Options *options = tox_options_new(NULL);
    tox_options_set_tcp_port(options, 33500);  // Use test port

    Tox *tox = tox_new(options, NULL);
    if (!tox) {
        printf("  FAILED: Could not create Tox instance\n");
        tox_options_free(options);
        return false;
    }

    // Enable access control - this uses internal function tcp_server_set_access_control_enabled
    tox_set_tcp_relay_access_control_enabled(tox, true);
    printf("  Enabled access control via public API\n");

    // Test adding keys - this uses internal function tcp_server_add_to_whitelist
    uint8_t key1[TOX_PUBLIC_KEY_SIZE];
    uint8_t key2[TOX_PUBLIC_KEY_SIZE];
    generate_test_key(key1, 100);
    generate_test_key(key2, 101);

    // Add keys
    bool result1 = tox_add_tcp_relay_to_whitelist(tox, key1);
    bool result2 = tox_add_tcp_relay_to_whitelist(tox, key2);

    if (!result1 || !result2) {
        printf("  FAILED: Could not add keys through public API\n");
        tox_kill(tox);
        tox_options_free(options);
        return false;
    }

    // Try to add the same key again (should still succeed - implementation doesn't prevent duplicates)
    bool result3 = tox_add_tcp_relay_to_whitelist(tox, key1);
    if (!result3) {
        printf("  FAILED: Should be able to 'add' the same key again\n");
        tox_kill(tox);
        tox_options_free(options);
        return false;
    }

    // Remove a key - this uses internal function tcp_server_remove_from_whitelist
    bool result4 = tox_remove_tcp_relay_from_whitelist(tox, key1);
    if (!result4) {
        printf("  FAILED: Could not remove key\n");
        tox_kill(tox);
        tox_options_free(options);
        return false;
    }

    // Try to remove the same key again (should fail)
    bool result5 = tox_remove_tcp_relay_from_whitelist(tox, key1);
    if (result5) {
        printf("  FAILED: Should not be able to remove same key twice\n");
        tox_kill(tox);
        tox_options_free(options);
        return false;
    }

    printf("  All public API tests (that exercise internal functions) passed\n");

    tox_kill(tox);
    tox_options_free(options);
    return true;
}

// Test array expansion functionality
bool test_array_expansion() {
    printf("Testing array expansion functionality...\n");
    
    Tox_Options *options = tox_options_new(NULL);
    tox_options_set_tcp_port(options, 33501);
    
    Tox *tox = tox_new(options, NULL);
    if (!tox) {
        printf("  FAILED: Could not create Tox instance\n");
        tox_options_free(options);
        return false;
    }

    tox_set_tcp_relay_access_control_enabled(tox, true);
    
    // Add enough keys to trigger array expansion (initial capacity is 8)
    const int initial_capacity = 8;
    const int test_size = initial_capacity * 2 + 1;  // Go beyond first expansion
    
    for (int i = 0; i < test_size; i++) {
        uint8_t key[TOX_PUBLIC_KEY_SIZE];
        generate_test_key(key, 200 + i);
        
        bool result = tox_add_tcp_relay_to_whitelist(tox, key);
        if (!result) {
            printf("  FAILED: Could not add key %d (capacity expansion failed)\n", i);
            tox_kill(tox);
            tox_options_free(options);
            return false;
        }
    }
    
    printf("  Successfully added %d keys (tested array expansion)\n", test_size);
    
    // Remove some keys to test array management
    for (int i = 0; i < test_size; i += 3) {  // Remove every 3rd key
        uint8_t key[TOX_PUBLIC_KEY_SIZE];
        generate_test_key(key, 200 + i);
        
        bool result = tox_remove_tcp_relay_from_whitelist(tox, key);
        if (!result) {
            printf("  FAILED: Could not remove key %d\n", i);
            tox_kill(tox);
            tox_options_free(options);
            return false;
        }
    }
    
    printf("  Successfully removed some keys from expanded array\n");
    
    tox_kill(tox);
    tox_options_free(options);
    return true;
}

// Test boundary conditions
bool test_boundary_conditions() {
    printf("Testing boundary conditions...\n");
    
    Tox_Options *options = tox_options_new(NULL);
    tox_options_set_tcp_port(options, 33502);
    
    Tox *tox = tox_new(options, NULL);
    if (!tox) {
        printf("  FAILED: Could not create Tox instance\n");
        tox_options_free(options);
        return false;
    }

    tox_set_tcp_relay_access_control_enabled(tox, true);
    
    // Test with all zero key
    uint8_t zero_key[TOX_PUBLIC_KEY_SIZE];
    memset(zero_key, 0, TOX_PUBLIC_KEY_SIZE);
    
    bool result = tox_add_tcp_relay_to_whitelist(tox, zero_key);
    if (!result) {
        printf("  FAILED: Could not add zero key\n");
        tox_kill(tox);
        tox_options_free(options);
        return false;
    }
    
    // Test with all 0xFF key
    uint8_t max_key[TOX_PUBLIC_KEY_SIZE];
    memset(max_key, 0xFF, TOX_PUBLIC_KEY_SIZE);
    
    result = tox_add_tcp_relay_to_whitelist(tox, max_key);
    if (!result) {
        printf("  FAILED: Could not add max key\n");
        tox_kill(tox);
        tox_options_free(options);
        return false;
    }
    
    // Remove both special keys
    result = tox_remove_tcp_relay_from_whitelist(tox, zero_key);
    if (!result) {
        printf("  FAILED: Could not remove zero key\n");
        tox_kill(tox);
        tox_options_free(options);
        return false;
    }
    
    result = tox_remove_tcp_relay_from_whitelist(tox, max_key);
    if (!result) {
        printf("  FAILED: Could not remove max key\n");
        tox_kill(tox);
        tox_options_free(options);
        return false;
    }
    
    printf("  Successfully tested special key values\n");
    
    tox_kill(tox);
    tox_options_free(options);
    return true;
}

int main() {
    printf("=== TCP Server Internal Unit Tests ===\n");
    
    int passed = 0;
    int total = 0;
    
    total++; if (test_public_api_for_internal_functions()) passed++;
    total++; if (test_array_expansion()) passed++;
    total++; if (test_boundary_conditions()) passed++;
    
    printf("\n=== Internal Unit Test Results: %d/%d tests passed ===\n", passed, total);
    
    if (passed == total) {
        printf("All internal tests PASSED!\n");
        return 0;
    } else {
        printf("Some internal tests FAILED!\n");
        return 1;
    }
}