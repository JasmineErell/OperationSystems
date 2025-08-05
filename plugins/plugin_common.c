#include "plugin_common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static plugin_context_t context;

// An entry function to thread that processes items from the queue
void* plugin_consumer_thread(void* arg)
{

    plugin_context_t* context = (plugin_context_t*)arg;
    // Check if the context is initialized
    if (!context) {
        fprintf(stderr, "Error: Plugin context is NULL.\n");
        return NULL;
    }

    if (!context->initialized) {
        fprintf(stderr, "Error: Plugin context is not initialized.\n");
        return NULL;
    }

    while (1) {
        char* item = consumer_producer_get(context->queue);
        fprintf(stderr, "[consumer] Got item: %s\n", item);

        if (!item) continue;

        int is_end = (strcmp(item, "<END>") == 0);
        fprintf(stderr, "[consumer] is_end = %d\n", is_end);

        const char* out = is_end ? item : context->process_function(item);
        fprintf(stderr, "[consumer] out: %s\n", out);

        if (context->next_place_work) {
            fprintf(stderr, "[consumer] Calling next_place_work...\n");
            context->next_place_work(out);
            fprintf(stderr, "[consumer] Returned from next_place_work.\n");
        } else {
            if (!is_end) {
                fprintf(stderr, "[consumer] Freeing output...\n");
                free((char*)out);
            }
        }

        free(item);

        if (is_end) {
            break;
        }
    }
    context->finished = 1;
    consumer_producer_signal_finished(context->queue);

    return NULL;

}

void log_error(plugin_context_t* context, const char* message)
{
    if (context == NULL) {
        fprintf(stderr, "Error: log_error received NULL context.\n");
        return;
    }

    if (message == NULL) {
        fprintf(stderr, "Error: log_error received NULL message.\n");
        return;
    }

    //SAFELY HANDLE NULL context->name
    const char* name = (context->name != NULL) ? context->name : "UNKNOWN";
    fprintf(stderr, "[ERROR][%s] - %s\n", name, message);
}

void log_info(plugin_context_t* context, const char* message)
{
    if (context == NULL) {
        fprintf(stderr, "Error: log_info received NULL context.\n");
        return;
    }

    if (message == NULL) {
        fprintf(stderr, "Error: log_info received NULL message.\n");
        return;
    }

    fprintf(stderr, "[INFO][%s] - %s\n", context->name, message);
}

// __attribute__((visibility("default")))
// const char* plugin_get_name(void)
// {
//     return "Common Plugin";
// }

__attribute__((visibility("default")))
const char* common_plugin_init(const char *(*process_function)(const char *), const char *name, int queue_size)
{
    memset(&context, 0, sizeof context); //Taking care of all fields
    context.name = name;
    context.process_function = process_function;

    if (!process_function) {
    log_error(&context, "common_plugin_init: process_function is NULL");
    return "process_function is NULL";
    }

    if (!name) {
        log_error(&context,"common_plugin_init: plugin name is NULL");
        return "plugin name is NULL";
    }
    if (queue_size <= 0) {
        log_error(&context, "common_plugin_init: queue_size must be > 0");   
        return "queue_size must be > 0";
    }

    //Allocating and initing queue for consumer producer
    context.queue = malloc(sizeof *context.queue);
    if (!context.queue) {
        log_error(&context, "malloc(queue) failed");
        return "malloc failed";
    }

    int rc = consumer_producer_init(context.queue, queue_size);
    if (rc != 0) {
        log_error(&context, "consumer_producer_init failed");
        consumer_producer_destroy(context.queue);  /* in case it half-initialised */
        free(context.queue);
        context.queue = NULL;
        return "queue init failed";
    }

    //Startnig consumer thread
    if (pthread_create(&context.consumer_thread, NULL,
                       plugin_consumer_thread, &context) != 0) {
        log_error(&context, "pthread_create failed");
        consumer_producer_destroy(context.queue);
        free(context.queue);
        context.queue = NULL;
        return "pthread_create failed";
    }


    //finial initialization
    context.initialized = 1;
    log_info(&context, "Plugin initialized successfully");
    return NULL;   

}

// // __attribute__((visibility("default")))
// const char* plugin_init(int queue_size)
// {
//     const char* (*plugin_transform)(const char*) = context.process_function;   
//     return common_plugin_init(plugin_transform, "<plugin_name>", queue_size);
// }

__attribute__((visibility("default")))
const char* plugin_fini(void) {
    if (!context.initialized) {
        return "Plugin not initialized";
    }
    
    consumer_producer_put(context.queue, "<END>");
    
    int res = pthread_join(context.consumer_thread, NULL);
    if (res != 0) {
        log_error(&context, "Failed to join plugin thread");
        return "Failed to join plugin thread";
    }
    
    consumer_producer_destroy(context.queue);
    free(context.queue);  
    context.queue = NULL;
    context.initialized = 0;  
    
    return NULL;
}

__attribute__((visibility("default")))
const char* plugin_place_work(const char* str)
{
    consumer_producer_t* queue = context.queue;

    if (str == NULL) {
        log_error(&context, "plugin_place_work received NULL string.");
        return NULL;
    }

    int result = consumer_producer_put(queue, str);
    if (result != 0) {
        log_error(&context, "Failed to put item in queue.");
        return "Failed to put item in queue";
    }

    log_info(&context, "Placed work in queue.");
    return NULL;
}

__attribute__((visibility("default")))
void plugin_attach(const char* (*next_place_work)(const char*))
{

    if (context.initialized == 1) {
        context.next_place_work = next_place_work;
        log_info(&context, "Next place_work function attached successfully.");
    } else {
        log_error(&context, "Cannot attach next place_work function: plugin not initialized.");
    }
}

__attribute__((visibility("default")))
const char* plugin_wait_finished(void)
{
    fprintf(stderr, "plugin_wait_finished called, DEBUG\n");
    if (!context.initialized) {
        log_error(&context, "plugin_wait_finished called before initialization.");
        return "Plugin not initialized";
    }

    if (context.finished) {
        log_info(&context, "Plugin has already finished processing.");
        return NULL;  
    }

    fprintf(stderr, "About to wait. Queue ptr: %p\n", (void*)context.queue);

    int res = consumer_producer_wait_finished(context.queue);
    if (res != 0) {
        log_error(&context, "Failed to wait for processing to finish.");
        return "Failed to wait for processing to finish";
    }

    context.finished = 1;
    log_info(&context, "Plugin finished processing successfully.");
    return NULL;  
}



//Some help functions:


//


