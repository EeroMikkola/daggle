#include "instance.h"
#include "resource_container.h"
#include "utility/return_macro.h"

#include <daggle/daggle.h>

daggle_error_code_t
daggle_plugin_register_node(
	daggle_instance_h instance,
	const char* node_type,
	daggle_node_declare_fn declare)
{
	REQUIRE_PARAMETER(instance);
	REQUIRE_PARAMETER(node_type);
	REQUIRE_PARAMETER(declare);

	instance_t* daggle_instance = instance;
	resource_container_t* resource_container
		= &daggle_instance->plugin_manager.res;

	ASSERT_NOT_NULL(resource_container, "Plugin manager contents is null");

	RETURN_IF_ERROR(resource_container_register_node(
		resource_container, node_type, declare));

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_plugin_register_type(
	daggle_instance_h instance,
	const char* type_name,
	daggle_data_clone_fn cloner,
	daggle_data_free_fn freer,
	daggle_data_serialize_fn serializer,
	daggle_data_deserialize_fn deserializer)
{
	REQUIRE_PARAMETER(instance);
	REQUIRE_PARAMETER(type_name);
	REQUIRE_PARAMETER(cloner);
	REQUIRE_PARAMETER(freer);
	REQUIRE_PARAMETER(serializer);
	REQUIRE_PARAMETER(deserializer);

	instance_t* daggle_instance = instance;
	resource_container_t* resource_container
		= &daggle_instance->plugin_manager.res;

	ASSERT_NOT_NULL(resource_container, "Plugin manager contents is null");

	RETURN_IF_ERROR(resource_container_register_type(resource_container,
		type_name,
		cloner,
		freer,
		serializer,
		deserializer));

	RETURN_STATUS(DAGGLE_SUCCESS);
}
