#include "arg_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Valid plugin names (must match those supported by your project)
static const char* valid_plugins[] = {
    "logger",
    "typewriter",
    "uppercaser",
    "rotator",
    "flipper",
    "expander"
};

#define NUM_VALID_PLUGINS (sizeof(valid_plugins) / sizeof(valid_plugins[0]))

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
    printf("\nArguments:\n");
    printf("  queue_size    Maximum number of items in each plugin's queue\n");
    printf("  plugin1..N    Names of plugins to load (without .so extension)\n");
    printf("\nAvailable plugins:\n");
    printf("  logger        - Logs all strings that pass through\n");
    printf("  typewriter    - Simulates typewriter effect with delays\n");
    printf("  uppercaser    - Converts strings to uppercase\n");
    printf("  rotator       - Move every character to the right. Last character moves to the beginning.\n");
    printf("  flipper       - Reverses the order of characters\n");
    printf("  expander      - Expands each character with spaces\n");
    printf("\nExample:\n");
    printf("  ./analyzer 20 uppercaser rotator logger\n");
}
