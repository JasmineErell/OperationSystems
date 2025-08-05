#include "arg_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

void run_test(const char* description, int argc, char** argv, int expected_exit_code) {
    pid_t pid = fork();
    if (pid == 0) {
        // === CHILD PROCESS ===

        // Silence stdout and stderr
        int dev_null = open("/dev/null", O_WRONLY);
        dup2(dev_null, STDOUT_FILENO); // suppress printf, print_usage
        dup2(dev_null, STDERR_FILENO); // suppress fprintf(stderr, ...)
        close(dev_null);

        // Run the function
        check_valid_args(argc, argv);

        exit(0); // if successful
    } else {
        // === PARENT PROCESS ===
        int status;
        waitpid(pid, &status, 0);
        int actual_exit_code = WEXITSTATUS(status);

        if (actual_exit_code == expected_exit_code) {
            printf("[PASS] %s\n", description);
        } else {
            printf("[FAIL] %s (expected exit code %d, got %d)\n",
                   description, expected_exit_code, actual_exit_code);
        }
    }
}


int main() {
    char* argv1[] = { "./analyzer", "20", "logger" };
    run_test("Valid: 20 logger", 3, argv1, 0);

    char* argv2[] = { "./analyzer", "012", "logger" };
    run_test("Invalid: leading zero", 3, argv2, 1);

    char* argv3[] = { "./analyzer", "20", "invalid_plugin" };
    run_test("Invalid: bad plugin name", 3, argv3, 1);

    char* argv4[] = { "./analyzer", "abc", "logger" };
    run_test("Invalid: non-digit queue size", 3, argv4, 1);

    char* argv5[] = { "./analyzer", "20" };
    run_test("Invalid: no plugin names", 2, argv5, 1);

    char* argv6[] = { "./analyzer" };
    run_test("Invalid: no arguments", 1, argv6, 1);

    return 0;
}
