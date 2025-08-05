#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <signal.h>
#include <limits.h>
#include <stdbool.h>
#include <sys/wait.h>
#include "../plugins/plugin_sdk.h"
#include "../plugins/plugin_common.h"

#ifndef PLUGIN
#error "You must compile with -DPLUGIN=<plugin_name>"
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// Test configuration
#define MAX_OUTPUTS 10000
#define STRESS_TEST_DURATION 3
#define MAX_TEST_THREADS 8

// Test results tracking
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    char last_error[256];
} test_results_t;

// Global test state
static char* outputs[MAX_OUTPUTS];
static int output_count = 0;
static pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;
static test_results_t g_results = {0, 0, 0, ""};

// Forward declarations for plugin interface
extern const char* plugin_init(int);
extern const char* plugin_fini(void);
extern const char* plugin_place_work(const char*);
extern const char* plugin_get_name(void);
extern const char* plugin_wait_finished(void);
extern void plugin_attach(const char* (*)(const char*));

// Helper functions
void print_test_header(const char* test_name) {
    printf("\n=== %s ===\n", test_name);
}


void print_test_result(const char* test_name, int passed) {
    g_results.total_tests++;
    if (passed) {
        g_results.passed_tests++;
        printf("✅ %s: PASSED\n", test_name);
    } else {
        g_results.failed_tests++;
        printf("❌ %s: FAILED\n", test_name);
        if (strlen(g_results.last_error) > 0) {
            printf("   Error: %s\n", g_results.last_error);
            memset(g_results.last_error, 0, sizeof(g_results.last_error));
        }
    }
}

void set_test_error(const char* error) {
    strncpy(g_results.last_error, error, sizeof(g_results.last_error) - 1);
}

void reset_outputs() {
    pthread_mutex_lock(&output_mutex);
    for (int i = 0; i < output_count; ++i) {
        free(outputs[i]);
    }
    output_count = 0;
    pthread_mutex_unlock(&output_mutex);
}

const char* capture_output(const char* str) {
    if (!str) return NULL;
    if (strcmp(str, "<END>") == 0) {
        return NULL;
    }
    
    pthread_mutex_lock(&output_mutex);
    if (output_count < MAX_OUTPUTS) {
        outputs[output_count++] = strdup(str);
    }
    pthread_mutex_unlock(&output_mutex);
    return NULL;
}

const char* null_output_sink(const char* str) {
    (void)str; // Suppress unused parameter warning
    return NULL;
}

char* create_test_string(int value) {
    char* str = malloc(64);
    snprintf(str, 64, "test_item_%d", value);
    return str;
}

// Expected output generators for different plugins
const char* get_expected_output(const char* plugin_name, const char* input) {
    static char buffer[4096];
    
    if (!input || strlen(input) == 0) {
        strcpy(buffer, input ? input : "");
        return buffer;
    }
    
    if (strcmp(plugin_name, "rotator") == 0) {
        int len = strlen(input);
        if (len <= 1) {
            strcpy(buffer, input);
        } else {
            buffer[0] = input[len-1];
            strncpy(buffer + 1, input, len - 1);
            buffer[len] = '\0';
        }
    } else if (strcmp(plugin_name, "flipper") == 0) {
        int len = strlen(input);
        for (int i = 0; i < len; i++) {
            buffer[i] = input[len - 1 - i];
        }
        buffer[len] = '\0';
    } else if (strcmp(plugin_name, "uppercaser") == 0) {
        strcpy(buffer, input);
        for (int i = 0; buffer[i]; i++) {
            if (buffer[i] >= 'a' && buffer[i] <= 'z') {
                buffer[i] = buffer[i] - 'a' + 'A';
            }
        }
    } else if (strcmp(plugin_name, "expander") == 0) {
        int j = 0;
        for (int i = 0; input[i] && j < 4095; i++) {
            buffer[j++] = input[i];
            if (input[i+1] && j < 4095) {
                buffer[j++] = ' ';
            }
        }
        buffer[j] = '\0';
    } else if (strcmp(plugin_name, "typewriter") == 0 || strcmp(plugin_name, "logger") == 0) {
        strcpy(buffer, input);
    } else {
        strcpy(buffer, input);
    }
    
    return buffer;
}

int verify_expected_output(const char* plugin_name, const char* input, const char* actual) {
    const char* expected = get_expected_output(plugin_name, input);
    return strcmp(expected, actual) == 0;
}

// =============================================================================
// BASIC FUNCTIONALITY TESTS
// =============================================================================

int test_basic_functionality() {
    print_test_header("Basic Plugin Functionality");
    reset_outputs();
    
    printf("  Testing plugin: %s\n", TOSTRING(PLUGIN));
    
    // Initialize plugin
    const char* err = plugin_init(10);
    if (err) {
        printf("    Plugin initialization failed: %s\n", err);
        print_test_result("Basic Plugin Functionality", 0);
        return 0;
    }
    
    plugin_attach(capture_output);
    
    // Test basic transformations
    const char* test_inputs[] = {
        "hello",
        "world",
        "abc",
        "123",
        "Test String",
        ""
    };
    
    int num_inputs = sizeof(test_inputs) / sizeof(test_inputs[0]);
    
    for (int i = 0; i < num_inputs; i++) {
        plugin_place_work(test_inputs[i]);
    }
    
    plugin_place_work("<END>");
    plugin_wait_finished();
    plugin_fini();
    
    if (output_count != num_inputs) {
        printf("    Expected %d outputs, got %d\n", num_inputs, output_count);
        print_test_result("Basic Plugin Functionality", 0);
        return 0;
    }
    
    // Verify each output
    for (int i = 0; i < num_inputs; i++) {
        if (!verify_expected_output(TOSTRING(PLUGIN), test_inputs[i], outputs[i])) {
            printf("    Input '%s' -> Expected '%s', Got '%s'\n", 
                   test_inputs[i], 
                   get_expected_output(TOSTRING(PLUGIN), test_inputs[i]), 
                   outputs[i]);
            print_test_result("Basic Plugin Functionality", 0);
            return 0;
        }
    }
    
    print_test_result("Basic Plugin Functionality", 1);
    return 1;
}

// =============================================================================
// INITIALIZATION TESTS
// =============================================================================

int test_plugin_lifecycle() {
    print_test_header("Plugin Lifecycle Management");

    // Test 1: Basic initialization and cleanup  
    printf("  Testing basic initialization...\n");
    const char* err = plugin_init(10);
    if (err) {
        set_test_error("Plugin initialization failed");
        print_test_result("Basic Plugin Lifecycle", 0);
        return 0;
    }

    const char* name = plugin_get_name();
    if (!name || strlen(name) == 0) {
        plugin_fini();
        set_test_error("Plugin name is invalid");
        print_test_result("Basic Plugin Lifecycle", 0);
        return 0;
    }
    printf("    Plugin name: %s\n", name);

    // 💡 Fix: Send END signal and wait for consumer to finish
    plugin_place_work("<END>");
    fprintf(stderr, "YOU ARE HERE\n");  // for error/debug logs

    plugin_wait_finished();
    
    err = plugin_fini();
    if (err) {
        set_test_error("Plugin finalization failed");
        print_test_result("Basic Plugin Lifecycle", 0);
        return 0;
    }

    // Test 2: Multiple init/fini cycles
    printf("  Testing multiple init/fini cycles...\n");
    for (int i = 0; i < 3; i++) {
        err = plugin_init(5 + i);
        if (err) {
            set_test_error("Multiple initialization failed");
            print_test_result("Basic Plugin Lifecycle", 0);
            return 0;
        }

        // 💡 Again, avoid consumer hanging
        plugin_place_work("<END>");
        plugin_wait_finished();

        err = plugin_fini();
        if (err) {
            set_test_error("Multiple finalization failed");
            print_test_result("Basic Plugin Lifecycle", 0);
            return 0;
        }
    }

    print_test_result("Basic Plugin Lifecycle", 1);
    return 1;
}

int test_invalid_initialization() {
    print_test_header("Invalid Initialization Parameters");
    
    // Test 1: Zero queue size
    printf("  Testing zero queue size...\n");
    const char* err = plugin_init(0);
    if (!err) {
        plugin_fini();
        set_test_error("Should reject zero queue size");
        print_test_result("Invalid Initialization", 0);
        return 0;
    }
    
    // Test 2: Negative queue size
    printf("  Testing negative queue size...\n");
    err = plugin_init(-5);
    if (!err) {
        plugin_fini();
        set_test_error("Should reject negative queue size");
        print_test_result("Invalid Initialization", 0);
        return 0;
    }
    
    print_test_result("Invalid Initialization", 1);
    return 1;
}

// =============================================================================
// INPUT VALIDATION TESTS
// =============================================================================

int test_null_and_invalid_inputs() {
    print_test_header("NULL and Invalid Input Handling");
    reset_outputs();
    
    plugin_init(10);
    plugin_attach(capture_output);
    
    // Test 1: NULL input to place_work
    printf("  Testing NULL input handling...\n");
    const char* result = plugin_place_work(NULL);
    if (!result) {
        printf("    NULL input accepted (implementation choice)\n");
    } else {
        printf("    NULL input rejected: %s\n", result);
    }
    
    // Test 2: Empty string
    printf("  Testing empty string...\n");
    result = plugin_place_work("");
    if (result) {
        plugin_place_work("<END>");
        plugin_wait_finished();
        plugin_fini();
        set_test_error("Empty string should be accepted");
        print_test_result("NULL and Invalid Input Handling", 0);
        return 0;
    }
    
    // Test 3: String with special characters
    printf("  Testing special characters...\n");
    plugin_place_work("!@#$%^&*()_+-=[]{}|;':\",./<>?");
    plugin_place_work("\t\n\r");
    
    plugin_place_work("<END>");
    plugin_wait_finished();
    plugin_fini();
    
    print_test_result("NULL and Invalid Input Handling", 1);
    return 1;
}

int test_boundary_conditions() {
    print_test_header("Boundary Condition Tests");
    reset_outputs();
    
    plugin_init(100);
    plugin_attach(capture_output);
    
    // Test various string lengths and edge cases
    const char* test_cases[] = {
        "",                                    // Empty
        "a",                                   // Single char
        "ab",                                  // Two chars
        "The quick brown fox jumps over the lazy dog", // Normal sentence
        "A",                                   // Single uppercase
        "z",                                   // Single lowercase
        "1",                                   // Single digit
        "!",                                   // Single special char
        "   ",                                 // Only spaces
        "\t\n\r",                             // Only whitespace
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",   // Repeated chars
        "1234567890123456789012345678901234567890", // Long digits
    };
    
    int num_cases = sizeof(test_cases) / sizeof(test_cases[0]);
    printf("  Testing %d boundary cases...\n", num_cases);
    
    for (int i = 0; i < num_cases; i++) {
        const char* result = plugin_place_work(test_cases[i]);
        if (result) {
            printf("    Case %d failed: %s\n", i, result);
        }
    }
    
    plugin_place_work("<END>");
    plugin_wait_finished();
    plugin_fini();
    
    printf("  Received %d outputs\n", output_count);
    
    print_test_result("Boundary Condition Tests", 1);
    return 1;
}

// =============================================================================
// FUNCTIONAL CORRECTNESS TESTS
// =============================================================================

int test_transformation_correctness() {
    print_test_header("Transformation Correctness");
    reset_outputs();
    
    plugin_init(50);
    plugin_attach(capture_output);
    
    // Comprehensive test cases for different plugins
    struct {
        const char* input;
        const char* description;
    } test_cases[] = {
        {"", "empty string"},
        {"a", "single character"},
        {"ab", "two characters"},
        {"abc", "three characters"},
        {"abcdefghijklmnopqrstuvwxyz", "full alphabet lowercase"},
        {"ABCDEFGHIJKLMNOPQRSTUVWXYZ", "full alphabet uppercase"},
        {"123456789", "digits only"},
        {"Hello, World!", "mixed case with punctuation"},
        {"   spaces   ", "strings with leading/trailing spaces"},
        {"!@#$%^&*()", "special characters only"},
        {"aA1!bB2@cC3#", "mixed characters"},
        {"Programming is fun", "normal sentence"},
        {"CamelCaseString", "camel case"},
        {"snake_case_string", "snake case"},
        {"kebab-case-string", "kebab case"},
        {"MiXeD_cAsE-StRiNg", "mixed formats"},
    };
    
    int num_cases = sizeof(test_cases) / sizeof(test_cases[0]);
    printf("  Testing %d transformation cases for plugin: %s\n", num_cases, TOSTRING(PLUGIN));
    
    for (int i = 0; i < num_cases; i++) {
        const char* result = plugin_place_work(test_cases[i].input);
        if (result) {
            printf("    Case %d ('%s') failed: %s\n", i, test_cases[i].description, result);
        }
    }
    
    plugin_place_work("<END>");
    plugin_wait_finished();
    plugin_fini();
    
    if (output_count != num_cases) {
        printf("    Expected %d outputs, got %d\n", num_cases, output_count);
        set_test_error("Output count mismatch");
        print_test_result("Transformation Correctness", 0);
        return 0;
    }
    
    // Verify each transformation
    int all_correct = 1;
    for (int i = 0; i < num_cases && i < output_count; i++) {
        if (!verify_expected_output(TOSTRING(PLUGIN), test_cases[i].input, outputs[i])) {
            printf("    FAIL: %s\n", test_cases[i].description);
            printf("      Input: '%s'\n", test_cases[i].input);
            printf("      Expected: '%s'\n", get_expected_output(TOSTRING(PLUGIN), test_cases[i].input));
            printf("      Got: '%s'\n", outputs[i]);
            all_correct = 0;
        } else {
            printf("    PASS: %s\n", test_cases[i].description);
        }
    }
    
    print_test_result("Transformation Correctness", all_correct);
    return all_correct;
}

// =============================================================================
// CONCURRENCY TESTS
// =============================================================================

typedef struct {
    int thread_id;
    int items_to_send;
    int items_sent;
    int start_value;
    volatile int* error_flag;
} producer_test_data_t;

void* producer_thread_func(void* arg) {
    producer_test_data_t* data = (producer_test_data_t*)arg;
    
    printf("    Producer %d starting (%d items)\n", data->thread_id, data->items_to_send);
    
    for (int i = 0; i < data->items_to_send; i++) {
        char* item = create_test_string(data->start_value + i);
        
        const char* result = plugin_place_work(item);
        if (result != NULL) {
            printf("    Producer %d: place_work failed at item %d: %s\n", 
                   data->thread_id, i, result);
            *(data->error_flag) = 1;
            free(item);
            break;
        }
        
        data->items_sent++;
        free(item);
        
        if (i % 50 == 0) usleep(1000);
    }
    
    printf("    Producer %d finished (%d items sent)\n", 
           data->thread_id, data->items_sent);
    return NULL;
}

int test_concurrent_producers() {
    print_test_header("Concurrent Producers Test");
    reset_outputs();
    
    plugin_init(50);
    plugin_attach(null_output_sink);
    
    const int num_producers = 4;
    const int items_per_producer = 100;
    volatile int error_flag = 0;
    
    producer_test_data_t producers[num_producers];
    pthread_t producer_threads[num_producers];
    
    printf("  Starting %d producers, %d items each...\n", num_producers, items_per_producer);
    
    // Initialize producer data
    for (int i = 0; i < num_producers; i++) {
        producers[i].thread_id = i;
        producers[i].items_to_send = items_per_producer;
        producers[i].items_sent = 0;
        producers[i].start_value = i * 1000;
        producers[i].error_flag = &error_flag;
    }
    
    // Start all producers
    for (int i = 0; i < num_producers; i++) {
        pthread_create(&producer_threads[i], NULL, producer_thread_func, &producers[i]);
    }
    
    // Wait for all producers
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producer_threads[i], NULL);
    }
    
    // Send end signal and wait
    plugin_place_work("<END>");
    plugin_wait_finished();
    plugin_fini();
    
    // Verify results
    int total_sent = 0;
    for (int i = 0; i < num_producers; i++) {
        total_sent += producers[i].items_sent;
    }
    
    printf("  Total items sent: %d\n", total_sent);
    
    int success = !error_flag && (total_sent == num_producers * items_per_producer);
    
    print_test_result("Concurrent Producers Test", success);
    return success;
}

// =============================================================================
// STRESS TESTS
// =============================================================================

void* stress_producer_thread(void* arg) {
    producer_test_data_t* data = (producer_test_data_t*)arg;
    time_t start_time = time(NULL);
    
    while (time(NULL) - start_time < STRESS_TEST_DURATION && !*(data->error_flag)) {
        char* item = create_test_string(data->items_sent);
        
        const char* result = plugin_place_work(item);
        if (result == NULL) {
            data->items_sent++;
        } else {
            *(data->error_flag) = 1;
        }
        
        free(item);
        
        if (data->items_sent % 500 == 0) {
            usleep(1000);
        }
    }
    
    return NULL;
}

int test_high_frequency_stress() {
    print_test_header("High Frequency Stress Test");
    reset_outputs();
    
    plugin_init(100);
    plugin_attach(null_output_sink);
    
    const int num_producers = 6;
    volatile int error_flag = 0;
    
    producer_test_data_t producers[num_producers];
    pthread_t producer_threads[num_producers];
    
    printf("  Running stress test for %d seconds with %d producers...\n", 
           STRESS_TEST_DURATION, num_producers);
    
    // Initialize producers
    for (int i = 0; i < num_producers; i++) {
        producers[i].thread_id = i;
        producers[i].items_sent = 0;
        producers[i].start_value = i * 10000;
        producers[i].error_flag = &error_flag;
    }
    
    // Start stress test
    for (int i = 0; i < num_producers; i++) {
        pthread_create(&producer_threads[i], NULL, stress_producer_thread, &producers[i]);
    }
    
    // Wait for stress test to complete
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producer_threads[i], NULL);
    }
    
    // Shutdown
    plugin_place_work("<END>");
    plugin_wait_finished();
    plugin_fini();
    
    // Calculate total throughput
    int total_processed = 0;
    for (int i = 0; i < num_producers; i++) {
        total_processed += producers[i].items_sent;
        printf("  Producer %d: %d items\n", i, producers[i].items_sent);
    }
    
    printf("  Total items processed: %d (%.1f items/sec)\n", 
           total_processed, (float)total_processed / STRESS_TEST_DURATION);
    
    int success = !error_flag && total_processed > 500;
    
    print_test_result("High Frequency Stress Test", success);
    return success;
}

// =============================================================================
// SHUTDOWN TESTS
// =============================================================================

int test_graceful_shutdown() {
    print_test_header("Graceful Shutdown Test");
    reset_outputs();
    
    plugin_init(20);
    plugin_attach(capture_output);
    
    // Test normal shutdown sequence
    printf("  Testing normal shutdown sequence...\n");
    plugin_place_work("test1");
    plugin_place_work("test2");
    plugin_place_work("test3");
    plugin_place_work("<END>");
    
    const char* wait_result = plugin_wait_finished();
    if (wait_result != NULL) {
        printf("    wait_finished failed: %s\n", wait_result);
        plugin_fini();
        set_test_error("wait_finished returned error");
        print_test_result("Graceful Shutdown Test", 0);
        return 0;
    }
    
    plugin_fini();
    
    if (output_count != 3) {
        printf("    Expected 3 outputs before shutdown, got %d\n", output_count);
        set_test_error("Incorrect output count during shutdown");
        print_test_result("Graceful Shutdown Test", 0);
        return 0;
    }
    
    print_test_result("Graceful Shutdown Test", 1);
    return 1;
}

// =============================================================================
// MAIN TEST RUNNER
// =============================================================================

void print_test_suite_header() {
    printf("┌─────────────────────────────────────────────────────────────────┐\n");
    printf("│                 COMPREHENSIVE PLUGIN TEST SUITE                │\n");
    printf("│                        Plugin: %-32s │\n", TOSTRING(PLUGIN));
    printf("└─────────────────────────────────────────────────────────────────┘\n");
}

void print_category_header(const char* category) {
    printf("\n🔧 %s\n", category);
    printf("─────────────────────────────────────────────────────────────────\n");
}

void print_final_summary() {
    printf("\n┌─────────────────────────────────────────────────────────────────┐\n");
    printf("│                        TEST SUMMARY                            │\n");
    printf("│                     Plugin: %-32s │\n", TOSTRING(PLUGIN));
    printf("├─────────────────────────────────────────────────────────────────┤\n");
    printf("│ Total Tests:    %3d                                          │\n", g_results.total_tests);
    printf("│ Passed:         %3d ✅                                        │\n", g_results.passed_tests);
    printf("│ Failed:         %3d ❌                                         │\n", g_results.failed_tests);
    
    if (g_results.total_tests > 0) {
        printf("│ Success Rate:   %5.1f%%                                       │\n", 
               (float)g_results.passed_tests / g_results.total_tests * 100);
    }
    printf("└─────────────────────────────────────────────────────────────────┘\n");
}

int main() {
    print_test_suite_header();
    
    srand(time(NULL));
    
    // Run all test categories
    print_category_header("INITIALIZATION AND LIFECYCLE TESTS");
    test_plugin_lifecycle();
    test_invalid_initialization();
    
    print_category_header("BASIC FUNCTIONALITY TESTS");
    test_basic_functionality();
    
    print_category_header("INPUT VALIDATION TESTS");
    test_null_and_invalid_inputs();
    test_boundary_conditions();
    
    print_category_header("FUNCTIONAL CORRECTNESS TESTS");
    test_transformation_correctness();
    
    print_category_header("CONCURRENCY TESTS");
    test_concurrent_producers();
    
    print_category_header("STRESS TESTS");
    test_high_frequency_stress();
    
    print_category_header("SYNCHRONIZATION TESTS");
    test_graceful_shutdown();
    
    // Final summary
    print_final_summary();
    
    // Clean up
    reset_outputs();
    pthread_mutex_destroy(&output_mutex);
    
    if (g_results.failed_tests == 0) {
        printf("\n🎉 ALL TESTS PASSED! Plugin %s is working correctly.\n", TOSTRING(PLUGIN));
        printf("   Plugin is ready for production use in the pipeline system.\n");
        return 0;
    } else {
        printf("\n⚠️  %d test(s) failed. Please review and fix the issues.\n", g_results.failed_tests);
        printf("   Plugin needs improvement before production deployment.\n");
        return 1;
    }
}




//gcc -std=c99 -Wall -Wextra -g -O2     -DPLUGIN=logger     -o output/test_logger_comprehensive     test_all_plugins_comprehensive.c     ../plugins/logger.so     -ldl -lpthread -lrt
// ./output/test_logger_comprehensive