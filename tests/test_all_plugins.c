#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "../plugins/plugin_sdk.h"
#include "../plugins/plugin_common.h"

#ifndef PLUGIN
#error "You must compile with -DPLUGIN=<plugin_name>"
#endif

#define MAX_OUTPUTS 10
#define ASSERT(msg, cond) \
    do { if (!(cond)) { fprintf(stderr, "❌ %s\n", msg); return 0; } } while (0)

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

static char* outputs[MAX_OUTPUTS];
static int output_count = 0;

void reset_outputs() {
    for (int i = 0; i < output_count; ++i) free(outputs[i]);
    output_count = 0;
}

const char* capture_output(const char* str) {
    if (strcmp(str, "<END>") == 0) return NULL;
    if (output_count < MAX_OUTPUTS)
        outputs[output_count++] = strdup(str);
    return NULL;
}

int check_outputs(const char* plugin, const char* inputs[], const char* expected[], int count) {
    for (int i = 0; i < count; ++i) {
        const char* actual = outputs[i];
        const char* input = inputs[i];
        const char* expect = expected[i];

        if (strcmp(actual, expect) == 0) {
            printf("[TEST] Input: \"%s\" → Output: \"%s\" ✅ PASS\n", input, actual);
        } else {
            printf("[TEST] Input: \"%s\" → Output: \"%s\" ❌ FAIL (expected \"%s\")\n",
                   input, actual, expect);
            return 0;
        }
    }
    return 1;
}

int simple_test_plugin(const char* plugin_name,
                       const char* (*init)(int),
                       const char* (*fini)(void),
                       const char* (*place_work)(const char*),
                       const char* (*get_name)(void),
                       const char* (*wait_finished)(void),
                       void (*attach)(const char* (*)(const char*))) {
    printf("\n🔧 Testing plugin: %s\n", plugin_name);
    reset_outputs();

    const char* err = init(10);
    if (err) {
        printf("❌ init failed: %s\n", err);
        return 0;
    }

    attach(capture_output);

    const char* inputs[] = { "abcd", "hello", "", "y", "Hi how are you?" };
    for (int i = 0; i < 5; ++i) place_work(inputs[i]);
    place_work("<END>");
    wait_finished();
    fini();

    ASSERT("output count != 5", output_count == 5);

    if (strcmp(plugin_name, "rotator") == 0) {
        const char* expected[] = { "dabc", "ohell", "", "y", "?Hi how are you" };
        return check_outputs(plugin_name, inputs, expected, 5);
    } else if (strcmp(plugin_name, "flipper") == 0) {
        const char* expected[] = { "dcba", "olleh", "", "y", "?uoy era woh iH" };
        return check_outputs(plugin_name, inputs, expected, 5);
    } else if (strcmp(plugin_name, "uppercaser") == 0) {
        const char* expected[] = { "ABCD", "HELLO", "", "Y", "HI HOW ARE YOU?" };
        return check_outputs(plugin_name, inputs, expected, 5);
    } else if (strcmp(plugin_name, "expander") == 0) {
        const char* expected[] = { "a b c d", "h e l l o", "", "y", "H i   h o w   a r e   y o u ?" };
        return check_outputs(plugin_name, inputs, expected, 5);
    } else if (strcmp(plugin_name, "typewriter") == 0) {
        const char* expected[] = { "abcd", "hello", "", "y", "Hi how are you?" };
        return check_outputs(plugin_name, inputs, expected, 5);
    } else if (strcmp(plugin_name, "logger") == 0) {
        const char* expected[] = { "abcd", "hello", "", "y", "Hi how are you?" };
        return check_outputs(plugin_name, inputs, expected, 5);
    } else {
        printf("⚠️ Unknown plugin name: %s\n", plugin_name);
        return 0;
    }

    return 1;
}

// Forward declarations for plugin interface
extern const char* plugin_init(int);
extern const char* plugin_fini(void);
extern const char* plugin_place_work(const char*);
extern const char* plugin_get_name(void);
extern const char* plugin_wait_finished(void);
extern void plugin_attach(const char* (*)(const char*));

int main() {
    int passed = 0, total = 0;

    total++;
    passed += simple_test_plugin(TOSTRING(PLUGIN),
                                  plugin_init, plugin_fini,
                                  plugin_place_work, plugin_get_name,
                                  plugin_wait_finished, plugin_attach);

    printf("\n[%s] Simple Test Summary: Passed %d / %d\n", TOSTRING(PLUGIN), passed, total);
    
// ------------------------------
// 🔍 ADVANCED TESTS FOR PLUGIN: TOSTRING(PLUGIN)
// ------------------------------

// Test NULL input
reset_outputs();
const char* null_output = plugin_place_work(NULL);
ASSERT("NULL input should not crash", null_output == NULL);

// Test long string
reset_outputs();
char* long_input = malloc(2048);
memset(long_input, 'x', 2047);
long_input[2047] = '\0';
plugin_place_work(long_input);
free(long_input);
plugin_place_work("<END>");
plugin_wait_finished();
plugin_fini();
ASSERT("long input processed", output_count == 1);

// Stress test with 1000 items
const int stress_count = 1000;
reset_outputs();
plugin_init(50);
plugin_attach(capture_output);

for (int i = 0; i < stress_count; ++i) {
    char buf[64];
    snprintf(buf, sizeof(buf), "item_%d", i);
    plugin_place_work(buf);
}
plugin_place_work("<END>");
plugin_wait_finished();
plugin_fini();
ASSERT("stress test processed all items", output_count == stress_count);

// Multithreaded input test
reset_outputs();
plugin_init(100);
plugin_attach(capture_output);

pthread_t threads[4];
void* thread_fn(void* arg) {
    int base = *(int*)arg;
    for (int i = 0; i < 100; ++i) {
        char buf[64];
        snprintf(buf, sizeof(buf), "thread%d_item%d", base, i);
        plugin_place_work(buf);
    }
    return NULL;
}

int bases[4] = {0, 1000, 2000, 3000};
for (int i = 0; i < 4; ++i) {
    pthread_create(&threads[i], NULL, thread_fn, &bases[i]);
}
for (int i = 0; i < 4; ++i) {
    pthread_join(threads[i], NULL);
}

plugin_place_work("<END>");
plugin_wait_finished();
plugin_fini();
ASSERT("multithreaded input handled", output_count == 400);

// Graceful shutdown test - wait should block until <END> is sent
reset_outputs();
plugin_init(10);
plugin_attach(capture_output);

volatile int waiter_unblocked = 0;
pthread_t waiter_thread;

void* waiter_func(void* _) {
    plugin_wait_finished();
    waiter_unblocked = 1;
    return NULL;
}

pthread_create(&waiter_thread, NULL, waiter_func, NULL);
usleep(200000); // Let waiter_func run and block

ASSERT("plugin_wait_finished should block before <END>", waiter_unblocked == 0);

// Now trigger shutdown
plugin_place_work("<END>");
pthread_join(waiter_thread, NULL);
ASSERT("plugin_wait_finished unblocked after <END>", waiter_unblocked == 1);
plugin_fini();

// Ensure immediate return after <END>
plugin_init(5);
plugin_attach(capture_output);
plugin_place_work("<END>");
const char* err = plugin_wait_finished();
ASSERT("plugin_wait_finished returns immediately after <END>", err == NULL);
plugin_fini();


    return (passed == total) ? 0 : 1;
}