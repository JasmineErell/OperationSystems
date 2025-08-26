#define _GNU_SOURCE
#include "plugin_common.h"
//#include "plugin_sdk.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> 

static const char* plugin_transform(const char* input) {
    if (input == NULL) return NULL;

    size_t len = strlen(input);
    if (len == 0) return strdup("");

    // Print "[typewriter] " with typewriter effect
    const char* prefix = "[typewriter] ";
    for (size_t i = 0; prefix[i] != '\0'; ++i) {
        printf("%c", prefix[i]);
        fflush(stdout);
        usleep(100000);  // 100ms delay for each character in prefix
    }

    // Print the input string with typewriter effect
    for (size_t i = 0; i < len; ++i) {
        printf("%c", input[i]);
        fflush(stdout);
        usleep(100000);  // 100ms delay for each character in input
    }

    printf("\n");  
    fflush(stdout);

    return strdup(input);  
}

__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "typewriter", queue_size);
}

__attribute__((visibility("default")))
const char* plugin_get_name(void) {
    return "typewriter";
}
