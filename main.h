#include "plugins/plugin_common.h"
#include "plugins/plugin_sdk.h"
#ifndef ARG_PARSER_H
#define ARG_PARSER_H

// typedef struct {
//     plugin_init_func_t init;
//     plugin_fini_func_t fini;
//     plugin_place_work_func_t place_work;
//     plugin_attach_func_t attach;
//     plugin_wait_finished_func_t wait_finished;
//     char* name;
//     void* handle;
// } plugin_handle_t;



void check_valid_args(int argc, char** argv);

// Helpers (public in case you want to test them)
int is_valid_plugin_name(const char* name);
int is_arg_starts_with_number(const char* str);
int are_valid_plugins(int argc, char** argv);
void print_usage(void);

#endif // ARG_PARSER_H