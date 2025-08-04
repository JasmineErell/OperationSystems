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
    return (passed == total) ? 0 : 1;
}