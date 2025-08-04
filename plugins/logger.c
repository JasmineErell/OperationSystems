#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plugin_common.h"
#include "plugin_sdk.h"


//Plugin logic
static const char* plugin_transform(const char* input) {
    if (input == NULL) return NULL;

    // Print to STDOUT with "[logger]" prefix
    printf("[logger] %s\n", input);
    fflush(stdout); 

    // Returns a string to next plugin (if any)
    return strdup(input); 
}


__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "logger", queue_size);
}

