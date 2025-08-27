#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>

int main(int argc, char** argv) {
    atexit(cleanup_temp_plugin_files);
    if (check_valid_args(argc, argv) == 0) {
        print_invalid_input();
        exit(1);
    }

    int queue_size = atoi(argv[1]);
    int plugin_count = argc - 2;
    char** plugin_names = &argv[2];

    plugin_handle_t* plugin_handlers = create_plugins_handle(plugin_names, plugin_count, queue_size);
    init_all_plugins(plugin_handlers, plugin_count, queue_size);
    attach_all_plugins(plugin_handlers, plugin_count);
    iterate_input_over_plugins(&plugin_handlers[0]);
    wait_for_all_plugins_to_finish(plugin_handlers, plugin_count);
    clean_plugins(plugin_handlers, plugin_count);
    printf("Pipeline shutdown complete\n");
    return 0;
}


//Help functions:



int check_valid_args(int argc, char** argv) {
    if (argc < 3) {
        return 0;
    }

    if (!is_arg_starts_with_number(argv[1])) {
        return 0;
    }


    return 1;
}

int is_arg_starts_with_number(const char* str) {
    if (str == NULL || *str == '\0') return 0;

    // Must start with a digit from 1 to 9
    if (str[0] < '1' || str[0] > '9') return 0;

    // All remaining characters must be digits
    for (int i = 1; str[i] != '\0'; ++i) {
        if (!isdigit((unsigned char)str[i])) return 0;
    }

    return 1;
}


void print_invalid_input(void) {
    fprintf(stderr, "Invalid input.\n");

    printf("Usage: ./analyzer <queue_size> <plugin1> <plugin2> ... <pluginN>\n\n");

    printf("Arguments:\n");
    printf("  queue_size   Maximum number of items in each plugin's queue\n");
    printf("  plugin1..N   Names of plugins to load (without .so extension)\n\n");

    printf("Available plugins:\n");
    printf("  logger     - Logs all strings that pass through\n");
    printf("  typewriter - Simulates typewriter effect with delays\n");
    printf("  uppercaser - Converts strings to uppercase\n");
    printf("  rotator    - Move every character to the right. Last character moves to the beginning.\n");
    printf("  flipper    - Reverses the order of characters\n");
    printf("  expander   - Expands each character with spaces\n\n");

    printf("Example:\n");
    printf("  ./analyzer 20 uppercaser rotator logger\n");
}


//Creating the plugin handles
plugin_handle_t* create_plugins_handle(char** plugin_names, int plugin_count, int queue_size) {
    plugin_handle_t* plugins = calloc(plugin_count, sizeof(plugin_handle_t));
    if (!plugins) {
        fprintf(stderr, "[ERROR] Failed to allocate plugin handles.\n");
        exit(1);
    }

    for (int i = 0; i < plugin_count; ++i) {
        // Count how many times this plugin has already appeared (before this point)
        int instance_num = 1;
        for (int j = 0; j < i; ++j) {
            if (strcmp(plugin_names[i], plugin_names[j]) == 0) {
                instance_num++;
            }
        }

        char original_filename[256];
        char actual_filename[256];
        snprintf(original_filename, sizeof(original_filename), "output/%s.so", plugin_names[i]);

        if (instance_num == 1) {
            strcpy(actual_filename, original_filename);
        } else {
            snprintf(actual_filename, sizeof(actual_filename), "output/%s_temp_%d_%d.so",
                     plugin_names[i], getpid(), instance_num);

            FILE* src = fopen(original_filename, "rb");
            FILE* dst = fopen(actual_filename, "wb");

            if (!src || !dst) {
                fprintf(stderr, "[ERROR] Failed to create temporary plugin copy\n");
                if (src) fclose(src);
                if (dst) fclose(dst);
                for (int j = 0; j < i; ++j) {
                    if (plugins[j].handle) dlclose(plugins[j].handle);
                }
                free(plugins);
                exit(1);
            }

            char buffer[4096];
            size_t bytes;
            while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
                fwrite(buffer, 1, bytes, dst);
            }

            fclose(src);
            fclose(dst);
        }

        void* handle = dlopen(actual_filename, RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            fprintf(stderr, "[ERROR] dlopen failed for %s: %s\n", actual_filename, dlerror());
            for (int j = 0; j < i; ++j) {
                if (plugins[j].handle) dlclose(plugins[j].handle);
            }
            free(plugins);
            exit(1);
        }

        plugin_handle_t* plugin = &plugins[i];
        plugin->handle = handle;
        plugin->name = plugin_names[i];
        plugin->init = dlsym(handle, "plugin_init");
        plugin->fini = dlsym(handle, "plugin_fini");
        plugin->place_work = dlsym(handle, "plugin_place_work");
        plugin->attach = dlsym(handle, "plugin_attach");
        plugin->wait_finished = dlsym(handle, "plugin_wait_finished");

        if (!plugin->init || !plugin->fini || !plugin->place_work ||
            !plugin->attach || !plugin->wait_finished) {
            fprintf(stderr, "[ERROR] dlsym error in %s: %s\n", plugin_names[i], dlerror());
            for (int j = 0; j <= i; ++j) {
                if (plugins[j].handle) dlclose(plugins[j].handle);
            }
            free(plugins);
            exit(1);
        }
    }

    return plugins;
}


// After creating plugin handles, we need to init each
void init_all_plugins(plugin_handle_t* plugins, int plugin_count, int queue_size) {
    for (int i = 0; i < plugin_count; ++i) {
        const char* init_error = plugins[i].init(queue_size);
        if (init_error != NULL) {
            fprintf(stderr, "[ERROR] Initialization failed for plugin '%s': %s\n",
                    plugins[i].name, init_error);

            // Clean up previously initialized plugins
            for (int j = 0; j <= i; ++j) {
                if (plugins[j].fini) {
                    plugins[j].fini();
                }
                if (plugins[j].handle) {
                    dlclose(plugins[j].handle);
                }
            }

            free(plugins);
            exit(1);
        }
    }
}

// After all plugins are initialized, we can attach them to each other
void attach_all_plugins(plugin_handle_t* plugins, int plugin_count) {
    for (int i = 0; i < plugin_count; ++i) {
        if (i < plugin_count - 1) {
            plugins[i].attach(plugins[i + 1].place_work);
        } else {
            // Attach NULL to last plugin 
            plugins[i].attach(NULL);
        }
    }
}


//Now when we have the "list", we can iterate it
#define MAX_LINE_LEN 1025 // 1024 +1 for fgets 
void iterate_input_over_plugins(plugin_handle_t* first_plugin) {
    char buffer[MAX_LINE_LEN];

    while (fgets(buffer, sizeof(buffer), stdin)) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }

        // Duplicate the string to send to plugin (plugin frees it at the end)
        char* input_copy = strdup(buffer);
        if (!input_copy) {
            fprintf(stderr, "[ERROR] Memory allocation failed for input.\n");
            exit(1);
        }

        // Send to first plugin
        const char* error = first_plugin->place_work(input_copy);
        if (error != NULL) {
            fprintf(stderr, "[ERROR] Failed to place work in plugin: %s\n", error);
            free(input_copy);
            exit(1);
        }

        if (strcmp(buffer, "<END>") == 0) {
            break;
        }
    }
}

//Wait for all plugins to finish (we get here after an <END> call breakes the loop in the finction above)
void wait_for_all_plugins_to_finish(plugin_handle_t* plugins, int plugin_count) {
    for (int i = 0; i < plugin_count; ++i) {
        const char* error = plugins[i].wait_finished();
        if (error != NULL) {
            fprintf(stderr, "[ERROR] Plugin '%s' failed to finish: %s\n", plugins[i].name, error);
            exit(1); // You can choose another code if needed
        }
    }
}
//When all finished we can finalize each plugin and cleanup
void clean_plugins(plugin_handle_t* plugins, int plugin_count) {
    for (int i = 0; i < plugin_count; ++i) {
        if (plugins[i].fini) {
            const char* error = plugins[i].fini();
            if (error != NULL) {
                fprintf(stderr, "[ERROR] Plugin '%s' failed to clean up: %s\n", plugins[i].name, error);
            }
        }

        if (plugins[i].handle) {
            dlclose(plugins[i].handle);
        }
    }

    free(plugins);  
}

// Cleanup temporary plugin files created during the run - helps me with double plugined
void cleanup_temp_plugin_files() {
    char cleanup_cmd[256];
    snprintf(cleanup_cmd, sizeof(cleanup_cmd), "rm -f output/*_temp_%d_*.so", getpid());
    system(cleanup_cmd);
}






