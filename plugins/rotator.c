#define _GNU_SOURCE

#include "plugin_common.h"  
//#include "plugin_sdk.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


// Plugin logic
static const char* plugin_transform(const char* input) {
    if (input == NULL) return NULL;

    size_t len = strlen(input);

    // Edge case: empty or single character string doesn't need rotation
    if (len <= 1) {
        return strdup(input);
    }

    char* result = malloc(len + 1); 
    if (!result) return NULL;

   
    for (size_t i = 1; i < len; ++i) {
        result[i] = input[i - 1];
    }

    result[0] = input[len - 1];
    result[len] = '\0';  // null terminate
    return result;       // plugin_common will free it
}


__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "rotator", queue_size);
}


__attribute__((visibility("default")))
const char* plugin_get_name(void) {
    return "rotator";
}
