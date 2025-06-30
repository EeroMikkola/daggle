#include "instance.h"
#include "plugin_manager.h"
#include "resource_container.h"
#include "stdlib.h"
#include "utility/return_macro.h"

#include <daggle/daggle.h>

daggle_error_code_t
daggle_instance_create(daggle_plugin_source_t** plugins, uint64_t num_plugins, daggle_instance_h* out_instance)
{
	REQUIRE_OUTPUT_PARAMETER(out_instance);

	instance_t* instance = malloc(sizeof *instance);
	REQUIRE_ALLOCATION_DAGGLE_SUCCESSFUL(instance);

	RETURN_IF_ERROR(plugin_manager_init(instance, plugins, num_plugins, &instance->plugin_manager));
	RETURN_IF_ERROR(executor_init(&instance->executor));

	*out_instance = instance;

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_instance_free(daggle_instance_h instance)
{
	REQUIRE_PARAMETER(instance);

	instance_t* instance_impl = instance;

	executor_destroy(&instance_impl->executor);

	plugin_manager_destroy(&instance_impl->plugin_manager);

	free(instance_impl);

	RETURN_STATUS(DAGGLE_SUCCESS);
}
