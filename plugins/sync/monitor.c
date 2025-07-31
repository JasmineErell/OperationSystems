#include "monitor.h"
#include <stdio.h>

//When calling it remember to crate space!
int monitor_init(monitor_t* monitor)
{
    if (monitor == NULL) {
        fprintf(stderr, "Error: monitor pointer is NULL.\n");
        return -1; // Monitor pointer is NULL
    }
    if (pthread_mutex_init(&monitor->mutex, NULL) != 0) {
        return -1; // Mutex initialization failed
    }

    if (pthread_cond_init(&monitor->condition, NULL) != 0) {
        pthread_mutex_destroy(&monitor->mutex); //because we already initialized the mutex
        return -1; // Condition variable initialization failed
    }

    monitor->signaled = 0; // Initialize the signaled flag
    return 0; // Success
}

//When calling it remember to free space!
void monitor_destroy(monitor_t* monitor)
{
    pthread_mutex_destroy(&monitor->mutex); // Destroy the mutex
    pthread_cond_destroy(&monitor->condition); // Destroy the condition variable
}

void monitor_signal(monitor_t* monitor)
{
    pthread_mutex_lock(&monitor->mutex); // Critical part starts
    monitor->signaled = 1; // Set the signaled flag
    pthread_cond_signal(&monitor->condition); // Signal the condition variable
    pthread_mutex_unlock(&monitor->mutex); // Critical part ends
}

void monitor_reset(monitor_t* monitor)
{
    pthread_mutex_lock(&monitor->mutex); // Critical part starts
    monitor->signaled = 0; // Reset the signaled flag
    pthread_mutex_unlock(&monitor->mutex); // Critical part ends
}

int monitor_wait(monitor_t* monitor)
{
    pthread_mutex_lock(&monitor->mutex); // Critical part starts
    while (!monitor->signaled) { // Wait until signaled
        pthread_cond_wait(&monitor->condition, &monitor->mutex); //Here the thread is asleep, making a room to ither thread do see the flag, and change it.
    }
    monitor->signaled = 0; // Reset the flag after being signaled
    pthread_mutex_unlock(&monitor->mutex); // Critical part ends (after waking up the thread aquires the mutex again)

    return 0; 
}




//gcc -c plugins/sync/monitor.c -o plugins/sync/monitor.o
