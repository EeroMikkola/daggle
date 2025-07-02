#pragma once

#include "resource_container.h"
#include "utility/dynamic_array.h"

#include <daggle/daggle.h>

typedef struct plugin_manager_s {
	dynamic_array_t plugin_instances;
	resource_container_t res;
	daggle_instance_h instance;
} plugin_manager_t;

daggle_error_code_t
plugin_manager_init(daggle_instance_h instance,
	daggle_plugin_source_t** plugins, uint64_t num_plugins,
	plugin_manager_t* out_plugin_manager);

void
plugin_manager_destroy(plugin_manager_t* plugin_manager);
