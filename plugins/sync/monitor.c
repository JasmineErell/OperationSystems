#include "monitor.h"
#include <stdio.h>

//When calling it remember to crate space!
int monitor_init(monitor_t* monitor)
{
if (monitor == NULL) {
    fprintf(stderr, "Error: monitor pointer is NULL.\n");
    return -1; // Monitor pointer is NULL
}

if (pthread_cond_init(&monitor->condition, NULL) != 0)
{
    return -1;
}

monitor->initialized = 1;
return 0;
}

//When calling it remember to free space!
void monitor_destroy(monitor_t* monitor)
{
    if (monitor == NULL) {
        fprintf(stderr, "Error: monitor_signal received NULL.\n");
        return;  
    }

    if (!monitor->initialized) {
        fprintf(stderr, "Error: monitor is not initialized.\n");
        return; // Monitor was not initialized
    }
    monitor->initialized = 0; // Mark as uninitialized
    pthread_cond_destroy(&monitor->condition); // Destroy the condition variable
}

void monitor_signal(monitor_t* monitor)
{
    //external lock already loceked, so we do not need to lock it here
   if (monitor == NULL) {
        fprintf(stderr, "Error: monitor_signal received NULL.\n");
        return;  
    }

    pthread_cond_signal(&monitor->condition); // Signal the condition variable
    
}

void monitor_reset(monitor_t* monitor)
//Make sure calling it while holding the lock
{
    if (monitor == NULL) {
        fprintf(stderr, "Error: monitor_signal received NULL.\n");
        return;  
    }
}

int monitor_wait(monitor_t* monitor, pthread_mutex_t* shared_mutex)
{
    if (monitor == NULL) {
        fprintf(stderr, "Error: monitor pointer is NULL.\n");
        return -1; // Monitor pointer is NULL
    }

    if (shared_mutex == NULL) {
        fprintf(stderr, "Error: shared mutex pointer is NULL.\n");
        return -1; // Shared mutex pointer is NULL
    }   

    return pthread_cond_wait(&monitor->condition, shared_mutex);
}




//gcc -c plugins/sync/monitor.c -o plugins/sync/monitor.o
