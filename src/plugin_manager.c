#include "plugin_manager.h"

#include "resource_container.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "utility/llist_queue.h"
#include "utility/return_macro.h"

daggle_error_code_t
plugin_manager_init(daggle_instance_h instance,
	daggle_plugin_source_t** plugins, uint64_t num_plugins, plugin_manager_t* out_plugin_manager)
{
	ASSERT_PARAMETER(instance);
	ASSERT_OUTPUT_PARAMETER(out_plugin_manager);

	out_plugin_manager->instance = instance;
	resource_container_init(&out_plugin_manager->res);

	dynamic_array_init(0, sizeof(daggle_plugin_interface_t),
		&out_plugin_manager->plugin_instances);

	// TODO: Validate dependencies
	
	for(uint64_t i = 0; i < num_plugins; ++i) {
		daggle_plugin_source_t* plugin = plugins[i];
		
		daggle_plugin_interface_t instance;
		plugin->load(plugin, &instance);
		
		dynamic_array_push(&out_plugin_manager->plugin_instances, &instance);
	}
	
	// TODO: Topologically sort plugins

	for(uint32_t i = 0; i < out_plugin_manager->plugin_instances.length; ++i) {
		daggle_plugin_interface_t* plugin
			= dynamic_array_at(&out_plugin_manager->plugin_instances, i);

		daggle_instance_h instance = out_plugin_manager->instance;
		plugin->init(instance, plugin->context);
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
		for(uint64_t j = 0; j < plugin_manager->plugin_descriptors.length; ++j) {
			daggle_plugin_source_t* other_item
				= dynamic_array_at(&plugin_manager->plugin_descriptors, j);

			for(char** dependency = other_item->dependencies; dependency; ++dependency) {
				if(strcmp(*dependency, (*item)->id) != 0) {
					continue;
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
