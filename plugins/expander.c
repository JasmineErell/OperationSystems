#define _GNU_SOURCE

#include "plugin_common.h"
#include "plugin_sdk.h"
#include <string.h>
#include <stdlib.h>

// Plugin logic
static const char* plugin_transform(const char* input) {
    if (input == NULL) return NULL;

    size_t len_word = strlen(input);
    size_t len_spaces = len_word-1;

    // Edge casempty or single character string doesn't need expanding
    if (len_word <= 1) {
        return strdup(input);
    }

    char* result = malloc(len_word + len_spaces + 1); 
    if (!result) return NULL;

   
    for (size_t i = 0; i < len_word; ++i) {
        result[i*2] = input[i];
        result[i*2+1] = ' ';  
    }

    result[len_spaces + len_word] = '\0';  // null terminate
    return result;       // plugin_common will free it
}

__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "expander", queue_size);
}

__attribute__((visibility("default")))
const char* plugin_get_name(void) {
    return "expander";
}

