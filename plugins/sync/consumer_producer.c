#define _GNU_SOURCE
#include "consumer_producer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>



int consumer_producer_init(consumer_producer_t* queue, int capacity)
{
    if (queue == NULL) {
        fprintf(stderr, "Error: queue pointer is NULL.\n");
        return -1;
    }

    if (capacity <= 0) {
        fprintf(stderr, "Error: Invalid capacity %d. Must be greater than 0.\n", capacity);
        return -1;
    }

    queue->items = malloc(sizeof(char*) * capacity);
    if (queue->items == NULL) {
       fprintf(stderr, "Error: Failed to allocate memory for items array.\n");
        return -1;
    }

    queue->capacity = capacity;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;

    // Initialize monitors
    if (monitor_init(&queue->not_full_monitor) != 0) // Monitor for producers
    {
        free(queue->items);// Preventing memory leak
        fprintf(stderr, "Error: Failed to initialize not_full_monitor.\n");
        return -1;
    }

    if (monitor_init(&queue->not_empty_monitor) != 0)
    {
        monitor_destroy(&queue->not_full_monitor);
        free(queue->items);// Preventing memory leak
        fprintf(stderr, "Error: Failed to initialize not_empty_monitor.\n");
        return -1;
    }

    if (monitor_init(&queue->finished_monitor) != 0) {
        monitor_destroy(&queue->not_empty_monitor);
        monitor_destroy(&queue->not_full_monitor);
        free(queue->items);// Preventing memory leak
        fprintf(stderr, "Error: Failed to initialize finished_monitor.\n");
        return -1;
    }

    if (pthread_mutex_init(&queue->shared_mutex, NULL) != 0) {
        monitor_destroy(&queue->not_empty_monitor);
        monitor_destroy(&queue->not_full_monitor);
        free(queue->items);
        fprintf(stderr, "Error: Failed to initialize shared_mutex.\n");
        return -1;
    }
    queue->initialized = 1; 
    queue->finished = 0;
    return 0;
}



void consumer_producer_destroy(consumer_producer_t* queue)
{
    if (queue == NULL) {
        fprintf(stderr, "Error: consumer_producer_destroy received NULL.\n");
        return;  
    }

    queue->capacity = 0;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;

    // Destroy monitors
    monitor_destroy(&queue->not_full_monitor);
    monitor_destroy(&queue->not_empty_monitor);
    monitor_destroy(&queue->finished_monitor);
    pthread_mutex_destroy(&queue->shared_mutex);


    // Free items array
    free(queue->items);
    queue->items = NULL; // Prevent dangling pointer
    queue->initialized = 0; // Mark as uninitialized
}

int consumer_producer_put(consumer_producer_t* queue, const char*item)
{
    int warned = 0; // Flag to track if we warned about full queue
    if (queue == NULL) {
        fprintf(stderr, "Error: consumer_producer_put received NULL queue.\n");
        return -1;
    }
    if (item == NULL) {
        fprintf(stderr, "Error: consumer_producer_put received NULL item.\n");
        return -1;
    }

    if (queue-> initialized == 0) {
        fprintf(stderr, "Error: consumer_producer_put called on uninitialized queue.\n");
        return -1;
    }
    pthread_mutex_lock(&queue->shared_mutex);
    
    while (queue->count == queue->capacity) {
        if (!warned) {
            fprintf(stderr, "[Producer] Waiting for space in the queue...\n");
            warned = 1; // Set flag to avoid multiple warnings
        }
        monitor_wait(&queue->not_full_monitor, &queue->shared_mutex);
    }

    queue->items[queue->tail] = strdup(item); 
    queue->tail = (queue->tail + 1) % (queue->capacity); // Cicly 
    queue->count++;

    fprintf(stderr,
            "[%ld][PUT] stored \"%s\"  cnt=%d h=%d t=%d\n",
            (long)pthread_self(), item,
            queue->count, queue->head, queue->tail);

    monitor_signal(&queue->not_empty_monitor); // Signal that the queue is not empty
    pthread_mutex_unlock(&queue->shared_mutex);
    return 0;
}

char* consumer_producer_get(consumer_producer_t* queue)
{
    int warned = 0; // Flag to track if we warned about empty queue
    char* item = NULL; // Initialize item to NULL, will be returned
    if (queue == NULL) {
        printf("Error: consumer_producer_get received NULL.\n");
        return item;
    }

    if (queue-> initialized == 0) {
        fprintf(stderr, "Error: consumer_producer_get called on uninitialized queue.\n");
        return NULL;
    }

    // Critical part 
    pthread_mutex_lock(&queue->shared_mutex); //Prevent race conditions while waiting
    // Keep waiting if the queue is empty (handles races conditions)
    while (queue->count == 0 && !queue->finished) {
        if (!warned) {
            fprintf(stderr, "[Consumer] Waiting for items in the queue...\n");
            warned = 1; // Set flag to avoid multiple warnings
        }
        monitor_wait(&queue->not_empty_monitor, &queue->shared_mutex);
    }

    if (queue->count == 0 && queue->finished) {// We stop waiting here and do not want te get an infinite loop
    pthread_mutex_unlock(&queue->shared_mutex);
    return NULL;
    }

    
    item = queue->items[queue->head]; // Get the item
    queue->head = (queue->head + 1) % (queue->capacity); // Cycle
    queue->count--;

    fprintf(stderr,
            "[%ld][GET] took   \"%s\"  cnt=%d h=%d t=%d\n",
            (long)pthread_self(), item,
            queue->count, queue->head, queue->tail);

    monitor_signal(&queue->not_full_monitor); // Signal that the queue is not full for the producer
    pthread_mutex_unlock(&queue->shared_mutex);

    return item;
}

//We will get it when the user writes <END>
void consumer_producer_signal_finished(consumer_producer_t* queue)
{
    if (queue == NULL) {
        fprintf(stderr, "Error: consumer_producer_signal_finished received NULL.\n");
        return;  
    }

    pthread_mutex_lock(&queue->shared_mutex);
    queue->finished = 1; // Set the finished flag
    monitor_signal(&queue->finished_monitor);
    pthread_mutex_unlock(&queue->shared_mutex);
    
}

//The main thread will wait for each plugin to finish processing, then the flag will turn to 1 and we will finish
int consumer_producer_wait_finished(consumer_producer_t* queue)
{
    if (queue == NULL) {
        fprintf(stderr, "Error: consumer_producer_wait_finished received NULL.\n");
        return -1; 
    }

    pthread_mutex_lock(&queue->shared_mutex);
    while (!queue->finished) { // Wait until finished
        printf("[Consumer] Waiting for processing to finish...\n");
        monitor_wait(&queue->finished_monitor, &queue->shared_mutex);
    }
    pthread_mutex_unlock(&queue->shared_mutex);

    return 0; 
}






