#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../plugins/sync/monitor.h"

int main() {
    printf("== Monitor Init Test ==\n");

    // Test 1: Pass NULL
    monitor_t* null_monitor = NULL;
    int res1 = monitor_init(null_monitor);
    printf("Test 1 (NULL monitor): returned %d (expected -1)\n", res1);
    assert(res1 == -1);

    // Test 2: Allocate monitor and test init
    monitor_t* monitor = malloc(sizeof(monitor_t));
    if (!monitor) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    int res2 = monitor_init(monitor);
    printf("Test 2 (valid monitor): returned %d (expected 0)\n", res2);
    assert(res2 == 0);

    // Optional cleanup
    monitor_destroy(monitor);
    free(monitor);

    printf("== Monitor Init Test Complete ==\n");
    return 0;
}