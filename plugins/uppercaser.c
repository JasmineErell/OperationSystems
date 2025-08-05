#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "plugin_common.h"
#include "plugin_sdk.h"


//Plugin logic
static const char* plugin_transform(const char* input) {
    if (input == NULL) return NULL;

    size_t len = strlen(input);
    char* result = malloc(len + 1);  // +1 for null terminator
    if (!result) return NULL;

    for (size_t i = 0; i < len; ++i) {
        result[i] = toupper((unsigned char)input[i]);
    }
    result[len] = '\0';  // null terminate
    return result;  // caller must free

}


__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "uppercaser", queue_size);
}

__attribute__((visibility("default")))
const char* plugin_get_name(void) {
    return "uppercaser";
}


