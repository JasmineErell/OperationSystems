#include "plugin_common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//static plugin_context_t context;

static plugin_context_t* context = NULL;


// An entry function to thread that processes items from the queue
void* plugin_consumer_thread(void* arg)
{
    plugin_context_t* context = (plugin_context_t*)arg;
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
        if (!item) continue;

        int is_end = (strcmp(item, "<END>") == 0);
        
        if (is_end) {
            if (context->next_place_work) {
                context->next_place_work(item);
            } else {
                free(item);
            }
            break;
        }

        // Process the item
        const char* out = context->process_function(item);
        
        if (context->next_place_work) {
            // Pass to next plugin
            context->next_place_work(out);
            // Free original if different from output
            if (out != item) {
                free(item);
            }
        } else {
            // Last plugin - print result
            if (strcmp(context->name, "typewriter") != 0) {
            printf("[%s] %s\n", context->name, out);
            fflush(stdout);
            }
            
            
            // Free strings properly
            if (out != item) {
                free((char*)out);
                free(item);
            } else {
                free(item);
            }
        }
    }

    context->finished = 1;
    consumer_producer_signal_finished(context->queue);
    return NULL;
}


void log_error(plugin_context_t* ctx, const char* message)
{
    if (ctx == NULL) {
        fprintf(stderr, "Error: log_error received NULL context.\n");
        return;
    }

    if (message == NULL) {
        fprintf(stderr, "Error: log_error received NULL message.\n");
        return;
    }

    const char* name = (ctx->name != NULL) ? ctx->name : "UNKNOWN";
    fprintf(stderr, "[ERROR][%s] - %s\n", name, message);
}


void log_info(plugin_context_t* ctx, const char* message)
{
    if (ctx == NULL) {
        fprintf(stderr, "Error: log_info received NULL context.\n");
        return;
    }

    if (message == NULL) {
        fprintf(stderr, "Error: log_info received NULL message.\n");
        return;
    }

    fprintf(stderr, "[INFO][%s] - %s\n", ctx->name, message);
}




__attribute__((visibility("default")))
const char* common_plugin_init(const char *(*process_function)(const char *), const char *name, int queue_size)
{
    // NEW: Allocate memory for this instance's context - JUST TRYING
    context = malloc(sizeof(plugin_context_t));
    if (!context) {
        fprintf(stderr, "[ERROR] Failed to allocate plugin context\n");
        return "Failed to allocate plugin context";
    }


    memset(context, 0, sizeof(plugin_context_t)); // Clear the allocated memory
    context->name = name;
    context->process_function = process_function;

    if (!process_function) {
        log_error(context, "common_plugin_init: process_function is NULL");
        // NEW: Clean up allocated memory on error
        free(context);
        context = NULL;
        return "process_function is NULL";
    }

    if (!name) {
        log_error(context, "common_plugin_init: plugin name is NULL");
        // NEW: Clean up allocated memory on error
        free(context);
        context = NULL;
        return "plugin name is NULL";
    }


    if (queue_size <= 0) {
        log_error(context, "common_plugin_init: queue_size must be > 0");
        // NEW: Clean up allocated memory on error
        free(context);
        context = NULL;
        return "queue_size must be > 0";
    }

    //Allocating and initing queue for consumer producer
    context->queue = malloc(sizeof(*context->queue));
    if (!context->queue) {
        log_error(context, "malloc(queue) failed");
        // NEW: Clean up allocated memory on error
        free(context);
        context = NULL;
        return "malloc failed";
    }

    int rc = consumer_producer_init(context->queue, queue_size);
    if (rc != 0) {
        log_error(context, "consumer_producer_init failed");
        consumer_producer_destroy(context->queue);
        free(context->queue);
        // NEW: Clean up allocated memory on error
        free(context);
        context = NULL;
        return "queue init failed";
    }

    //Startnig consumer thread
    if (pthread_create(&context->consumer_thread, NULL, plugin_consumer_thread, context) != 0) {
        log_error(context, "pthread_create failed");
        consumer_producer_destroy(context->queue);
        free(context->queue);

        // NEW: Clean up allocated memory on error
        free(context);
        context = NULL;
        return "pthread_create failed";
    }


    //finial initialization
    context->initialized = 1;
    log_info(context, "Plugin initialized successfully");
    return NULL;
}


__attribute__((visibility("default")))
const char* plugin_fini(void) {
    if (context == NULL) {
        return "Plugin context is NULL";
    }

    if (!context->initialized) {
        return "Plugin not initialized";
    }
    
    consumer_producer_put(context->queue, "<END>");
    
    int res = pthread_join(context->consumer_thread, NULL);
    if (res != 0) {
        log_error(context, "Failed to join plugin thread");
        return "Failed to join plugin thread";
    }
    
    consumer_producer_destroy(context->queue);
    free(context->queue);
    
    // NEW: Free the context itself and reset pointer
    free(context);
    context = NULL;
    
    return NULL;
}

__attribute__((visibility("default")))
const char* plugin_place_work(const char* str)
{
    if (context == NULL) {
        log_error(context, "plugin_place_work called before initialization.");
        return "Plugin not initialized";
    }
    if (!context->queue) {
        log_error(context, "plugin_place_work called with NULL queue.");
        return "Queue not initialized";
    }

    if (str == NULL) {
        log_error(context, "plugin_place_work received NULL string.");
        return NULL;
    }

    int result = consumer_producer_put(context->queue, str);
    if (result != 0) {
        log_error(context, "Failed to put item in queue.");
        return "Failed to put item in queue";
    }

    log_info(context, "Placed work in queue.");
    return NULL;
}

__attribute__((visibility("default")))
void plugin_attach(const char* (*next_place_work)(const char*))
{
    // NEW: Check if context pointer is valid
    if (!context) {
        fprintf(stderr, "[ERROR] Cannot attach: plugin not initialized\n");
        return;
    }

    if (context->initialized == 1) {
        context->next_place_work = next_place_work;
        log_info(context, "Next place_work function attached successfully.");
    } else {
        log_error(context, "Cannot attach next place_work function: plugin not initialized.");
    }
}

__attribute__((visibility("default")))
const char* plugin_wait_finished(void)
{
    // CHANGED: Check if context pointer is valid
    if (!context || !context->initialized) {
        fprintf(stderr, "[ERROR] plugin_wait_finished called before initialization\n");
        return "Plugin not initialized";
    }

    if (context->finished) {
        log_info(context, "Plugin has already finished processing.");
        return NULL;
    }

    int res = consumer_producer_wait_finished(context->queue);
    if (res != 0) {
        log_error(context, "Failed to wait for processing to finish.");
        return "Failed to wait for processing to finish";
    }

    context->finished = 1;
    log_info(context, "Plugin finished processing successfully.");
    return NULL;
}




