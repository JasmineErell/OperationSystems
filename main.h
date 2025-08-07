#ifndef MAIN_H
#define MAIN_H

typedef const char* (*plugin_init_func_t)(int);
typedef const char* (*plugin_fini_func_t)(void);
typedef const char* (*plugin_place_work_func_t)(const char*);
typedef void (*plugin_attach_func_t)(const char* (*)(const char*));
typedef const char* (*plugin_wait_finished_func_t)(void);
void cleanup_temp_plugin_files();

typedef struct {
    plugin_init_func_t init;
    plugin_fini_func_t fini;
    plugin_place_work_func_t place_work;
    plugin_attach_func_t attach;
    plugin_wait_finished_func_t wait_finished;
    char* name;
    void* handle;
} plugin_handle_t;


void check_valid_args(int argc, char** argv);
int is_arg_starts_with_number(const char* str);
int is_valid_plugin_name(const char* name);
int are_valid_plugins(int argc, char** argv);
void print_usage(void);
plugin_handle_t* create_plugins_handle(char** plugin_names, int plugin_count, int queue_size);
void init_all_plugins(plugin_handle_t* plugins, int plugin_count, int queue_size);
void attach_all_plugins(plugin_handle_t* plugins, int plugin_count);
void iterate_input_over_plugins(plugin_handle_t* first_plugin); 
void wait_for_all_plugins_to_finish(plugin_handle_t* plugins, int plugin_count);
void clean_plugins(plugin_handle_t* plugins, int plugin_count);

#endif // MAIN_H
