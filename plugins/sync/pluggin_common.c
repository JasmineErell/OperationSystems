#include "common_pluggins.h"

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
    char *item = consumer_producer_get(context->queue);
    if (!item) continue;               
    int is_end = (strcmp(item, "<END>") == 0);

    // move forward
    const char *out = is_end ? item: ctx->process_function(item);

    if (context->next_place_work)
        context->next_place_work(out);
    else                         /* last plugin in the chain */
        free((char*)out);

    if (is_end)                  /* sentinel seen → exit loop */
        break;
    }

    context->finished = 1;
    consumer_producer_signal_finished(context->queue);

    return NULL;

}

void log_error(plugin_context_t* context, const char* message)
{
    if (context == NULL) {
        fprintf(stderr, "Error: log_error received NULL context .\n");
        return;
    }

    if (message == NULL) {
        fprintf(stderr, "Error: log_error received NULL message.\n");
        return;
    }

    fprintf(stderr, "[ERROR][%s] - %s\n", context->name, message);
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

const char* plugin_get_name(void)
{
    return "Common Plugin";
}

/**
* Initialize the common plugin infrastructure with the specified queue size
* @param process_function Plugin-specific processing function
* @param name Plugin name
* @param queue_size Maximum number of items that can be queued
* @return NULL on success, error message on failure
*/

int common_plugin_init(const char *(*process_function)(const char *),const char *name, int queue_size)
{
    // validating arguments
    if (!process_function) {
        fprintf(stderr, "common_plugin_init: process_function is NULL\n");
        return 1;
    }
    if (!name) {
        fprintf(stderr, "common_plugin_init: plugin name is NULL\n");
        return 1;
    }
    if (queue_size <= 0) {
        fprintf(stderr, "common_plugin_init: queue_size must be > 0\n");
        return 1;
    }

    // setting up the plugin context
    consumer_producer_t *queue = malloc(sizeof *queue);
    if (!queue) {
        perror("common_plugin_init: malloc queue");
        return 1;
    }
    if (consumer_producer_init(queue, queue_size) != 0) {
        fprintf(stderr, "common_plugin_init: consumer_producer_init failed\n");
        free(queue);
        return 1;
    }

    //allocating  the plugin context 
    plugin_context_t *context = calloc(1, sizeof *context);
    if (!context) {
        perror("common_plugin_init: calloc context");
        consumer_producer_destroy(queue);
        free(queue);
        return 1;
    }

    context->name             = name;          
    context->queue            = queue;
    context->process_function = process_function;
    context->next_place_work  = NULL;
    context->initialized      = 1;
    context->finished         = 0;

    g_plugin_ctx = context;                       
    return 0;
}

int plugin_init(int queue_size)
{
    return common_plugin_init(plugin_transform, "<plugin_name>", queue_size);
}