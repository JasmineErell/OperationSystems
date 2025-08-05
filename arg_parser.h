#ifndef ARG_PARSER_H
#define ARG_PARSER_H

// Entry point to check command-line arguments
void check_valid_args(int argc, char** argv);

// Utility functions (can be used in tests)
int is_valid_plugin_name(const char* name);
int are_valid_plugins(int argc, char** argv);
int is_arg_starts_with_number(const char* str);
void print_usage(void);

#endif // ARG_PARSER_H
