#include "plugin_common.h"
#include "plugin_sdk.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> 

static const char* plugin_transform(const char* input) {
    if (input == NULL) return NULL;

    size_t len = strlen(input);
    if (len == 0) return strdup("");

    printf("[typewriter] ");
    fflush(stdout);

    for (size_t i = 0; i < len; ++i) {
        printf("%c", input[i]);
        fflush(stdout);
        usleep(100000);  
    }

    printf("\n");  
    fflush(stdout);

    return strdup(input);  
}

__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "typewriter", queue_size);
}
