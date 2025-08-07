#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_VALID_PLUGINS (sizeof(valid_plugins) / sizeof(valid_plugins[0]))



// --------------------------------------
// main()
// --------------------------------------
int main(int argc, char** argv) {
    atexit(cleanup_temp_plugin_files);
    check_valid_args(argc, argv);

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


// Valid plugin names (must match those supported by your project)
static const char* valid_plugins[] = {
    "logger",
    "typewriter",
    "uppercaser",
    "rotator",
    "flipper",
    "expander"
};


void check_valid_args(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "[ERROR] Not enough arguments.\n");
        print_usage();
        exit(1);
    }

    if (!is_arg_starts_with_number(argv[1])) {
        fprintf(stderr, "[ERROR] Invalid queue size: must be a positive integer with no leading zeros.\n");
        print_usage();
        exit(1);
    }

    if (!are_valid_plugins(argc, argv)) {
        fprintf(stderr, "[ERROR] Invalid plugin name(s): must be one or more from the supported list.\n");
        print_usage();
        exit(1);
    }
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

int is_valid_plugin_name(const char* name) {
    for (size_t i = 0; i < NUM_VALID_PLUGINS; ++i) {
        if (strcmp(name, valid_plugins[i]) == 0) return 1;
    }
    return 0;
}

int are_valid_plugins(int argc, char** argv) {
    for (int i = 2; i < argc; ++i) {
        if (!is_valid_plugin_name(argv[i])) return 0;
    }
    return 1;
}

void print_usage(void) {
    printf("Usage: ./analyzer <queue_size> <plugin1> <plugin2> ... <pluginN>\n");
    printf("Available plugins: logger, typewriter, uppercaser, rotator, flipper, expander\n");
    printf("Example: ./analyzer 10 logger typewriter\n");

}

//Creating the plugin handles
//Replace your create_plugins_handle function with this version:

plugin_handle_t* create_plugins_handle(char** plugin_names, int plugin_count, int queue_size) {
    plugin_handle_t* plugins = calloc(plugin_count, sizeof(plugin_handle_t));
    if (!plugins) {
        fprintf(stderr, "[ERROR] Failed to allocate plugin handles.\n");
        exit(1);
    }

    // Count instances of each plugin type
    int instance_counters[NUM_VALID_PLUGINS] = {0};

    for (int i = 0; i < plugin_count; ++i) {
        // Find which plugin type this is
        int plugin_type_index = -1;
        for (size_t j = 0; j < NUM_VALID_PLUGINS; ++j) {
            if (strcmp(plugin_names[i], valid_plugins[j]) == 0) {
                plugin_type_index = j;
                break;
            }
        }

        // Increment instance counter for this plugin type
        instance_counters[plugin_type_index]++;
        int instance_num = instance_counters[plugin_type_index];

        char original_filename[256];
        char actual_filename[256];
        
        snprintf(original_filename, sizeof(original_filename), "output/%s.so", plugin_names[i]);
        
        if (instance_num == 1) {
            // First instance - use original file
            strcpy(actual_filename, original_filename);
        } else {
            // Additional instances - create temporary copy with unique name
            snprintf(actual_filename, sizeof(actual_filename), "output/%s_temp_%d_%d.so", 
                     plugin_names[i], getpid(), instance_num);
            
            // Copy the original file to create a unique instance
            FILE* src = fopen(original_filename, "rb");
            FILE* dst = fopen(actual_filename, "wb");
            
            if (!src || !dst) {
                fprintf(stderr, "[ERROR] Failed to create temporary plugin copy\n");
                if (src) fclose(src);
                if (dst) fclose(dst);
                
                // Cleanup previously allocated plugins
                for (int j = 0; j < i; ++j) {
                    if (plugins[j].handle) {
                        dlclose(plugins[j].handle);
                    }
                }
                free(plugins);
                exit(1);
            }
            
            // Copy file contents
            char buffer[4096];
            size_t bytes;
            while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
                fwrite(buffer, 1, bytes, dst);
            }
            
            fclose(src);
            fclose(dst);
        }

        // Load the .so file (original or copy)
        void* handle = dlopen(actual_filename, RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            fprintf(stderr, "[ERROR] dlopen failed for %s: %s\n", actual_filename, dlerror());
            print_usage();

            // Cleanup
            for (int j = 0; j < i; ++j) {
                if (plugins[j].handle) {
                    dlclose(plugins[j].handle);
                }
            }
            free(plugins);
            exit(1);
        }

        plugin_handle_t* plugin = &plugins[i];
        plugin->handle = handle;
        plugin->name = plugin_names[i];  // Keep original name for output
        plugin->init = dlsym(handle, "plugin_init");
        plugin->fini = dlsym(handle, "plugin_fini");
        plugin->place_work = dlsym(handle, "plugin_place_work");
        plugin->attach = dlsym(handle, "plugin_attach");
        plugin->wait_finished = dlsym(handle, "plugin_wait_finished");

        if (!plugin->init || !plugin->fini || !plugin->place_work ||
            !plugin->attach || !plugin->wait_finished) {
            fprintf(stderr, "[ERROR] dlsym error in %s: %s\n", plugin_names[i], dlerror());

            for (int j = 0; j <= i; ++j) {
                if (plugins[j].handle) {
                    dlclose(plugins[j].handle);
                }
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
//NOT SURE HOW IT WORKS WITH THE LAST PLUGIN!!!
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

    free(plugins);  // Free the array of structs
}

// Cleanup temporary plugin files created during the run - helps me with double plugined!
void cleanup_temp_plugin_files() {
    char cleanup_cmd[256];
    snprintf(cleanup_cmd, sizeof(cleanup_cmd), "rm -f output/*_temp_%d_*.so", getpid());
    system(cleanup_cmd);
}






