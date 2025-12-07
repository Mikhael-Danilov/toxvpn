#include "c-toxcore/toxcore/tox.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

int main() {
    printf("Testing TCP Relay Access Control Implementation\n");

    // Initialize Tox options with TCP server enabled
    Tox_Options *options = tox_options_new(NULL);
    if (!options) {
        printf("Failed to create Tox options\n");
        return 1;
    }

    // Enable TCP server on port 33445
    tox_options_set_tcp_port(options, 44345);
    
    // Create Tox instance
    Tox *tox = tox_new(options, NULL);
    if (!tox) {
        printf("Failed to create Tox instance\n");
        tox_options_free(options);
        return 1;
    }

    printf("Tox instance created successfully\n");

    Tox_Err_Get_Port error;

    // Check if TCP server is available
    if (tox_self_get_tcp_port(tox, &error) == 0) {
        printf("TCP server not available\n");
        tox_kill(tox);
        tox_options_free(options);
        return 1;
    }

    printf("TCP server is running on port: %d\n", tox_self_get_tcp_port(tox, &error));

    // Generate some test public keys
    uint8_t test_public_key[TOX_PUBLIC_KEY_SIZE];
    for (int i = 0; i < TOX_PUBLIC_KEY_SIZE; i++) {
        test_public_key[i] = i % 256;
    }

    // Enable access control
    tox_set_tcp_relay_access_control_enabled(tox, true);
    printf("Access control enabled\n");

    // Test adding to whitelist
    bool add_result = tox_add_tcp_relay_to_whitelist(tox, test_public_key);
    printf("Add to whitelist result: %s\n", add_result ? "SUCCESS" : "FAILURE");

    // Test removing from whitelist
    bool remove_result = tox_remove_tcp_relay_from_whitelist(tox, test_public_key);
    printf("Remove from whitelist result: %s\n", remove_result ? "SUCCESS" : "FAILURE");

    // Add again for testing
    add_result = tox_add_tcp_relay_to_whitelist(tox, test_public_key);
    printf("Add to whitelist (second time) result: %s\n", add_result ? "SUCCESS" : "FAILURE");

    // Disable access control
    tox_set_tcp_relay_access_control_enabled(tox, false);
    printf("Access control disabled\n");

    // Test functions when access control is disabled
    uint8_t another_public_key[TOX_PUBLIC_KEY_SIZE];
    for (int i = 0; i < TOX_PUBLIC_KEY_SIZE; i++) {
        another_public_key[i] = (i + 100) % 256;
    }

    add_result = tox_add_tcp_relay_to_whitelist(tox, another_public_key);
    printf("Add to whitelist with access control disabled result: %s\n", add_result ? "SUCCESS" : "FAILURE");

    tox_kill(tox);
    tox_options_free(options);

    printf("TCP relay access control test completed\n");
    return 0;
}