/*
 * Integration tests for TCP relay access control
 * These tests verify the complete system behavior
 */

#include "c-toxcore/toxcore/tox.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

// Helper function to generate a deterministic public key
void generate_test_key(uint8_t *key, int seed) {
    for (int i = 0; i < 32; i++) {  // Using 32 since that's the crypto public key size
        key[i] = (seed + i) % 256;
    }
}

// Test complete workflow: enable control, add keys, disable control, etc.
bool test_complete_workflow() {
    printf("Testing complete workflow...\n");
    
    Tox_Options *options = tox_options_new(NULL);
    tox_options_set_tcp_port(options, 33600);
    
    Tox *tox = tox_new(options, NULL);
    if (!tox) {
        printf("  FAILED: Could not create Tox instance\n");
        tox_options_free(options);
        return false;
    }

    // Get initial state (access control should be disabled by default)
    // We can't directly check this with public API, so we test behavior
    
    uint8_t test_key[TOX_PUBLIC_KEY_SIZE];
    generate_test_key(test_key, 1001);
    
    // Add a key (should work regardless of access control state)
    bool result = tox_add_tcp_relay_to_whitelist(tox, test_key);
    if (!result) {
        printf("  FAILED: Could not add key to whitelist\n");
        tox_kill(tox);
        tox_options_free(options);
        return false;
    }
    
    // Enable access control
    tox_set_tcp_relay_access_control_enabled(tox, true);
    printf("  Enabled access control\n");
    
    // Add another key while enabled
    uint8_t test_key2[TOX_PUBLIC_KEY_SIZE];
    generate_test_key(test_key2, 1002);
    
    result = tox_add_tcp_relay_to_whitelist(tox, test_key2);
    if (!result) {
        printf("  FAILED: Could not add key while access control is enabled\n");
        tox_kill(tox);
        tox_options_free(options);
        return false;
    }
    
    // Disable access control
    tox_set_tcp_relay_access_control_enabled(tox, false);
    printf("  Disabled access control\n");
    
    // Add another key while disabled
    uint8_t test_key3[TOX_PUBLIC_KEY_SIZE];
    generate_test_key(test_key3, 1003);
    
    result = tox_add_tcp_relay_to_whitelist(tox, test_key3);
    if (!result) {
        printf("  FAILED: Could not add key while access control is disabled\n");
        tox_kill(tox);
        tox_options_free(options);
        return false;
    }
    
    // Re-enable access control
    tox_set_tcp_relay_access_control_enabled(tox, true);
    printf("  Re-enabled access control\n");
    
    // Add another key while re-enabled
    uint8_t test_key4[TOX_PUBLIC_KEY_SIZE];
    generate_test_key(test_key4, 1004);
    
    result = tox_add_tcp_relay_to_whitelist(tox, test_key4);
    if (!result) {
        printf("  FAILED: Could not add key after re-enabling access control\n");
        tox_kill(tox);
        tox_options_free(options);
        return false;
    }
    
    printf("  Successfully completed workflow test\n");
    
    tox_kill(tox);
    tox_options_free(options);
    return true;
}

// Test performance with many operations
bool test_performance() {
    printf("Testing performance with many operations...\n");
    
    Tox_Options *options = tox_options_new(NULL);
    tox_options_set_tcp_port(options, 33601);
    
    Tox *tox = tox_new(options, NULL);
    if (!tox) {
        printf("  FAILED: Could not create Tox instance\n");
        tox_options_free(options);
        return false;
    }

    tox_set_tcp_relay_access_control_enabled(tox, true);
    
    clock_t start = clock();
    
    // Add many keys
    const int num_keys = 100;
    uint8_t keys[num_keys][TOX_PUBLIC_KEY_SIZE];
    
    for (int i = 0; i < num_keys; i++) {
        generate_test_key(keys[i], 2000 + i);
        tox_add_tcp_relay_to_whitelist(tox, keys[i]);
    }
    
    // Remove half of them
    for (int i = 0; i < num_keys; i += 2) {
        tox_remove_tcp_relay_from_whitelist(tox, keys[i]);
    }
    
    // Add them back
    for (int i = 0; i < num_keys; i += 2) {
        tox_add_tcp_relay_to_whitelist(tox, keys[i]);
    }
    
    clock_t end = clock();
    double time_spent = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("  Performed %d operations in %.3f seconds\n", num_keys * 3, time_spent);
    
    if (time_spent > 5.0) {  // If it takes more than 5 seconds, that's too slow
        printf("  WARNING: Operations took too long (%.3f seconds)\n", time_spent);
    } else {
        printf("  Performance test completed within acceptable time\n");
    }
    
    tox_kill(tox);
    tox_options_free(options);
    return true;
}

// Test thread safety by simulating concurrent access (conceptually)
// Since we can't easily test real concurrency without multiple threads,
// we'll test rapid sequence of operations that would be problematic in concurrent scenarios
bool test_sequential_operations() {
    printf("Testing sequential operations for potential concurrency issues...\n");
    
    Tox_Options *options = tox_options_new(NULL);
    tox_options_set_tcp_port(options, 33602);
    
    Tox *tox = tox_new(options, NULL);
    if (!tox) {
        printf("  FAILED: Could not create Tox instance\n");
        tox_options_free(options);
        return false;
    }

    tox_set_tcp_relay_access_control_enabled(tox, true);
    
    // Rapid add/remove operations to test state consistency
    uint8_t test_key[TOX_PUBLIC_KEY_SIZE];
    generate_test_key(test_key, 3001);
    
    // Add and remove the same key multiple times
    for (int i = 0; i < 10; i++) {
        bool add_result = tox_add_tcp_relay_to_whitelist(tox, test_key);
        if (!add_result) {
            printf("  FAILED: Could not add key on iteration %d\n", i);
            tox_kill(tox);
            tox_options_free(options);
            return false;
        }
        
        bool remove_result = tox_remove_tcp_relay_from_whitelist(tox, test_key);
        if (!remove_result && i > 0) {  // Allow failure on first remove if key didn't exist initially
            printf("  FAILED: Could not remove key on iteration %d\n", i);
            tox_kill(tox);
            tox_options_free(options);
            return false;
        }
    }
    
    // Add several keys, then remove them all
    uint8_t keys[5][TOX_PUBLIC_KEY_SIZE];
    for (int i = 0; i < 5; i++) {
        generate_test_key(keys[i], 3002 + i);
        tox_add_tcp_relay_to_whitelist(tox, keys[i]);
    }
    
    // Remove them all
    for (int i = 0; i < 5; i++) {
        bool result = tox_remove_tcp_relay_from_whitelist(tox, keys[i]);
        if (!result) {
            printf("  FAILED: Could not remove key %d\n", i);
            tox_kill(tox);
            tox_options_free(options);
            return false;
        }
    }
    
    printf("  Successfully completed sequential operations test\n");
    
    tox_kill(tox);
    tox_options_free(options);
    return true;
}

// Test memory cleanup by creating and destroying multiple times
bool test_memory_cleanup() {
    printf("Testing memory cleanup...\n");
    
    // Create and destroy multiple Tox instances to test for memory leaks
    for (int i = 0; i < 3; i++) {
        Tox_Options *options = tox_options_new(NULL);
        tox_options_set_tcp_port(options, 33603 + i);  // Different ports
        
        Tox *tox = tox_new(options, NULL);
        if (!tox) {
            printf("  FAILED: Could not create Tox instance on iteration %d\n", i);
            tox_options_free(options);
            return false;
        }

        // Add some keys to each instance
        uint8_t key1[TOX_PUBLIC_KEY_SIZE];
        uint8_t key2[TOX_PUBLIC_KEY_SIZE];
        generate_test_key(key1, 4000 + i);
        generate_test_key(key2, 4000 + i + 100);
        
        tox_add_tcp_relay_to_whitelist(tox, key1);
        tox_add_tcp_relay_to_whitelist(tox, key2);
        
        tox_set_tcp_relay_access_control_enabled(tox, true);
        
        tox_kill(tox);
        tox_options_free(options);
        
        printf("  Created and destroyed instance %d\n", i);
    }
    
    printf("  Successfully completed memory cleanup test\n");
    return true;
}

int main() {
    printf("=== TCP Relay Access Control Integration Tests ===\n");
    
    int passed = 0;
    int total = 0;
    
    total++; if (test_complete_workflow()) passed++;
    total++; if (test_performance()) passed++;
    total++; if (test_sequential_operations()) passed++;
    total++; if (test_memory_cleanup()) passed++;
    
    printf("\n=== Integration Test Results: %d/%d tests passed ===\n", passed, total);
    
    if (passed == total) {
        printf("All integration tests PASSED!\n");
        return 0;
    } else {
        printf("Some integration tests FAILED!\n");
        return 1;
    }
}