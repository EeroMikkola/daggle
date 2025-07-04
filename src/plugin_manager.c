#include "plugin_manager.h"

#include "daggle/daggle.h"
#include "resource_container.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "utility/return_macro.h"

// Check if a plugin with a given ID exists in the plugin source list.
bool
prv_is_plugin_id_in_plugins(char* plugin, daggle_plugin_source_t** plugins,
	uint64_t num_plugins)
{
	for (uint64_t i = 0; i < num_plugins; ++i) {
		daggle_plugin_source_t* plugin_source = plugins[i];

		if (strcmp(plugin, plugin_source->id) == 0) {
			return true;
		}
	}

	return false;
}

// Check if every dependency of a plugin is included in the plugin source list.
bool
prv_check_plugin_dependencies_met(daggle_plugin_source_t* plugin,
	daggle_plugin_source_t** plugins, uint64_t num_plugins)
{
	// If the plugin does not have dependencies, it is good to go.
	if (!plugin->dependencies) {
		return true;
	}

	// Note: the dependency string consists of plugin IDs separated by commas.

	// Buffer to store the current dependency ID.
	char buffer[64];

	// Pointer to the first character of the currently checked dependency ID.
	char* current_dep = plugin->dependencies;

	bool should_stop = false;
	while (!should_stop) {
		// Get pointer to the next delimiter
		char* delimiter = strchr(current_dep, ',');

		// If there are no delimiters left, the current dependency is the last.
		if (!delimiter) {
			delimiter = strchr(current_dep, '\0');
			should_stop = true;
		}

		// Copy the dependency ID to the buffer and add a null terminator.
		memcpy(buffer, current_dep, delimiter - current_dep);
		buffer[delimiter - current_dep] = '\0';

		// Check if the depended plugin is in the plugin source list.
		if (!prv_is_plugin_id_in_plugins(buffer, plugins, num_plugins)) {
			LOG_FMT(LOG_TAG_ERROR,
				"Plugin \"%s\" required by plugin \"%s\" not found", buffer,
				plugin->id);
			return false;
		}

		// Change the current dependency to the next plugin in the string.
		current_dep = delimiter + 1;
	}

	return true;
}

// Check if all the plugins have their dependencies met.
bool
prv_check_plugins_dependencies_met(daggle_plugin_source_t** plugins,
	uint64_t num_plugins)
{
	for (uint64_t i = 0; i < num_plugins; ++i) {
		daggle_plugin_source_t* plugin_source = plugins[i];

		if (!prv_check_plugin_dependencies_met(plugin_source, plugins,
				num_plugins)) {
			return false;
		}
	}

	return true;
}

daggle_error_code_t
plugin_manager_init(daggle_instance_h instance,
	daggle_plugin_source_t** plugins, uint64_t num_plugins,
	plugin_manager_t* out_plugin_manager)
{
	ASSERT_PARAMETER(instance);
	ASSERT_OUTPUT_PARAMETER(out_plugin_manager);

	if (!prv_check_plugins_dependencies_met(plugins, num_plugins)) {
		RETURN_STATUS(DAGGLE_ERROR_MISSING_DEPENDENCY);
	}

	// Set the reference to daggle instance.
	out_plugin_manager->instance = instance;

	// Initialize resource container.
	resource_container_init(&out_plugin_manager->res);

	// Initialize the plugin instance array to store the loaded plugins.
	dynamic_array_init(num_plugins, sizeof(daggle_plugin_interface_t),
		&out_plugin_manager->plugin_instances);

	// Load plugin sources to get the plugin instances.
	for (uint64_t i = 0; i < num_plugins; ++i) {
		daggle_plugin_source_t* plugin_source = plugins[i];

		daggle_plugin_interface_t instance;
		plugin_source->load(plugin_source, &instance);

		dynamic_array_push(&out_plugin_manager->plugin_instances, &instance);
	}

	// TODO: Topologically sort plugins

	for (uint32_t i = 0; i < out_plugin_manager->plugin_instances.length; ++i) {
		daggle_plugin_interface_t* plugin_instance
			= dynamic_array_at(&out_plugin_manager->plugin_instances, i);

		// Initialize the plugin instance. This is where the plugins then call
		// daggle_plugin_register_X functions to register their contents.
		plugin_instance->init(instance, plugin_instance->context);
	}

	RETURN_STATUS(DAGGLE_SUCCESS);
}

void
plugin_manager_destroy(plugin_manager_t* plugin_manager)
{
	ASSERT_PARAMETER(plugin_manager);

	dynamic_array_destroy(&plugin_manager->plugin_instances);
	resource_container_destroy(&plugin_manager->res);
}

/*daggle_error_code_t
prv_plugin_manager_sort_topologically(plugin_manager_t* plugin_manager,
	daggle_plugin_source_t*** out_results,
	uint64_t* out_result_len)
{
	REQUIRE_PARAMETER(plugin_manager);
	REQUIRE_OUTPUT_PARAMETER(out_results);
	REQUIRE_OUTPUT_PARAMETER(out_result_len);

	if(plugin_manager->plugin_descriptors.length == 0) {
		*out_results = NULL;
		*out_result_len = 0;
		RETURN_STATUS(DAGGLE_ERROR_MEMORY_ALLOCATION);
	}

	daggle_plugin_source_t** sorted
		= malloc(sizeof(daggle_plugin_source_t*)
				 * plugin_manager->plugin_descriptors.length);
	if(!sorted) {
		*out_results = NULL;
		*out_result_len = 0;
		RETURN_STATUS(DAGGLE_ERROR_MEMORY_ALLOCATION);
	}

	// Initialize sorted as NULL
	for(int i = 0; i < plugin_manager->plugin_descriptors.length; ++i) {
		sorted[i] = NULL;
	}

	uint64_t* dependency_count
		= malloc(plugin_manager->plugin_descriptors.length);
	if(!dependency_count) {
		free(sorted);

		*out_results = NULL;
		*out_result_len = 0;
		RETURN_STATUS(DAGGLE_ERROR_MEMORY_ALLOCATION);
	}

	llist_queue_t queue;
	llist_queue_init(&queue);

	// Calculate initial dependency counts
	for(uint64_t i = 0; i < plugin_manager->plugin_descriptors.length; ++i) {
		daggle_plugin_source_t* item
			= dynamic_array_at(&plugin_manager->plugin_descriptors, i);

		// Count number of dependencies
		dependency_count[i] = 0;
		for(char** dependency = item->dependencies; dependency; ++dependency) {
			++dependency_count[i];
		}

		// If the required daggle ABI version does not match, increment
		// dependency count This blocks it from being enqueued. Plugins with
		// wrong ABI version should not even get this far.
		if((item)->abi != DAGGLE_ABI_VERSION) {
			LOG_FMT(LOG_TAG_WARN,
				"Plugin %s does not support this daggle ABI version",
				(item)->id);
			++dependency_count[i];
		}

		// Initialize the queue with plugins that have zero dependencies
		if(dependency_count[i] > 0) {
			continue;
		}

		llist_queue_enqueue(&queue, &item);
	}

	uint64_t sorted_count = 0;

	// Process the queue for topological sorting
	while(true) {
		daggle_plugin_source_t** item = NULL;
		llist_queue_dequeue(&queue, (void**)&item);

		if(item == NULL) {
			break;
		}

		sorted[sorted_count++] = *item;

		// Reduce the dependency count of each dependent plugin
		for(uint64_t j = 0; j < plugin_manager->plugin_descriptors.length; ++j)
{ daggle_plugin_source_t* other_item =
dynamic_array_at(&plugin_manager->plugin_descriptors, j);

			for(char** dependency = other_item->dependencies; dependency;
++dependency) { if(strcmp(*dependency, (*item)->id) != 0) { continue;
				}

				if(--dependency_count[j] > 0) {
					continue;
				}

				// Found a dependency on the current plugin
				llist_queue_enqueue(&queue, &other_item);
				break;
			}
		}
	}

	for(uint64_t i = 0; i < plugin_manager->plugin_descriptors.length; ++i) {
		daggle_plugin_source_t* item
			= dynamic_array_at(&plugin_manager->plugin_descriptors, i);

		if(dependency_count[i] > 0) {
			LOG_FMT(LOG_TAG_WARN,
				"Failed to load plugin %s, its dependants won't be loaded",
				item->id);
		}
	}

	free(dependency_count);
	llist_queue_destroy(&queue);

	*out_results = sorted;
	*out_result_len = sorted_count;

	RETURN_STATUS(DAGGLE_SUCCESS);
}*/
