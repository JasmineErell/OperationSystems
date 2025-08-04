#ifndef PLUGIN_SDK_H
#define PLUGIN_SDK_H

#ifdef __cplusplus
extern "C" {
#endif

// Get the plugin's name
const char* plugin_get_name(void);

// Initialize the plugin with the specified queue size
const char* plugin_init(int queue_size);

// Finalize the plugin - terminate thread gracefully
const char* plugin_fini(void);

// Place work (a string) into the plugin's queue
const char* plugin_place_work(const char* str);

// Attach this plugin to the next plugin in the chain
void plugin_attach(const char* (*next_place_work)(const char*));

// Wait until the plugin has finished processing all work and is ready to shutdown
const char* plugin_wait_finished(void);

#ifdef __cplusplus
}
#endif

#endif // PLUGIN_SDK_H
