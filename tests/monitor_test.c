#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include "../plugins/sync/monitor.h"

void* waiter_thread(void* arg) {
    monitor_t* monitor = (monitor_t*)arg;
    printf("[Thread] Waiting on monitor...\n");
    int res = monitor_wait(monitor);
    assert(res == 0);
    printf("[Thread] Woke up from monitor_wait()\n");
    return NULL;
}

void test_monitor_init() {
    printf("\n== Test: monitor_init ==\n");

    // Test 1: NULL input
    assert(monitor_init(NULL) == -1);

    // Test 2: Valid input
    monitor_t* m = malloc(sizeof(monitor_t));
    assert(m != NULL);
    assert(monitor_init(m) == 0);
    monitor_destroy(m);
    free(m);
}

void test_monitor_wait_and_signal() {
    printf("\n== Test: monitor_wait + monitor_signal ==\n");

    monitor_t* m = malloc(sizeof(monitor_t));
    assert(m != NULL);
    assert(monitor_init(m) == 0);

    pthread_t t;
    pthread_create(&t, NULL, waiter_thread, m);

    sleep(1); // Give time for thread to block on wait
    printf("[Main] Signaling monitor...\n");
    monitor_signal(m);

    pthread_join(t, NULL);
    monitor_destroy(m);
    free(m);
}

void test_monitor_reset() {
    printf("\n== Test: monitor_reset ==\n");

    monitor_t* m = malloc(sizeof(monitor_t));
    assert(m != NULL);
    assert(monitor_init(m) == 0);

    monitor_signal(m);
    assert(m->signaled == 1);
    monitor_reset(m);
    assert(m->signaled == 0);

    monitor_destroy(m);
    free(m);
}

void test_monitor_destroy_null() {
    printf("\n== Test: monitor_destroy(NULL) ==\n");
    monitor_destroy(NULL); // Should print error, not crash
}

int main() {
    printf("=== Starting Monitor Tests ===\n");
    test_monitor_init();
    test_monitor_wait_and_signal();
    test_monitor_reset();
    test_monitor_destroy_null();
    printf("=== All Monitor Tests Passed ===\n");
    return 0;
}
