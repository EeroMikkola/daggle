#include "resource_container.h"

#include "instance.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"
#include "utility/dynamic_array.h"
#include "utility/hash.h"
#include "utility/return_macro.h"
#include "stddef.h"
#include <string.h>

void
resource_container_init(resource_container_t* resource_container)
{
	ASSERT_PARAMETER(resource_container);

	// Hot reloading could be supported, if these were to use pointers.
	// It is currently not reliably possible, as the address would change
	// if the array is resized.
	dynamic_array_init(0, sizeof(node_info_t), &resource_container->nodes);
	dynamic_array_init(0, sizeof(type_info_t), &resource_container->types);
}

void
resource_container_destroy(resource_container_t* resource_container)
{
	ASSERT_PARAMETER(resource_container);

	for (uint64_t i = 0; i < resource_container->nodes.length; ++i) {
		node_info_t* info = dynamic_array_at(&resource_container->nodes, i);
		free((char*)info->name_hash.name);
	}

	dynamic_array_destroy(&resource_container->nodes);

	for (uint64_t i = 0; i < resource_container->types.length; ++i) {
		type_info_t* info = dynamic_array_at(&resource_container->types, i);
		free((char*)info->name_hash.name);
	}

	dynamic_array_destroy(&resource_container->types);
}

daggle_error_code_t
daggle_plugin_register_node(daggle_instance_h instance,
	const char* node_type, daggle_node_declare_fn declare)
{
	REQUIRE_PARAMETER(instance);
	REQUIRE_PARAMETER(node_type);
	REQUIRE_PARAMETER(declare);

	node_info_t info = {
		.name_hash = {
			.name = strdup(node_type), 
			.hash = fnv1a_32(node_type)
		},
		.declare = declare,
	};

	// LOG_FMT_COND_DEBUG("Registered node %s", node_type);

	resource_container_t* container = &((instance_t*)instance)->plugin_manager.res;
	RETURN_IF_ERROR(dynamic_array_push(&container->nodes, &info));

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_plugin_register_type(daggle_instance_h instance,
	const char* type_name, daggle_data_clone_fn cloner,
	daggle_data_free_fn freer, daggle_data_serialize_fn serializer,
	daggle_data_deserialize_fn deserializer)
{
	REQUIRE_PARAMETER(instance);
	REQUIRE_PARAMETER(type_name);
	REQUIRE_PARAMETER(cloner);
	REQUIRE_PARAMETER(freer);
	REQUIRE_PARAMETER(serializer);
	REQUIRE_PARAMETER(deserializer);

	type_info_t info = {
		.name_hash = { 
			.name = strdup(type_name), 
			.hash = fnv1a_32(type_name) 
		},
		.cloner = cloner,
		.freer = freer,
		.serializer = serializer,
		.deserializer = deserializer,
	};

	// LOG_FMT_COND_DEBUG("Registered type %s (%u)", info.name, info.hash);

	resource_container_t* container = &((instance_t*)instance)->plugin_manager.res;
	RETURN_IF_ERROR(dynamic_array_push(&container->types, &info));

	RETURN_STATUS(DAGGLE_SUCCESS);
}

// Search dynamic_array_t of structs where the member at offset is name_with_hash_t
daggle_error_code_t
prv_name_hash_array_get_item(dynamic_array_t* array, uint64_t offset,
	const char* name, void** out_item)
{
	ASSERT_PARAMETER(array);
	ASSERT_PARAMETER(name);
	ASSERT_OUTPUT_PARAMETER(out_item);

	ASSERT_TRUE(array->stride >= sizeof(name_with_hash_t), 
		"Array should contain struct with name_with_hash_t member");

	const uint32_t search_hash = fnv1a_32(name);

	for (uint64_t i = 0; i < array->length; ++i) {
		void* item = dynamic_array_at(array, i);
		name_with_hash_t* nh = item + offset;

		if (nh->hash == search_hash
			&& !strcmp(name, nh->name)) {
			*out_item = item;
			RETURN_STATUS(DAGGLE_SUCCESS);
		}
	}

	// Not found.
	LOG_FMT(LOG_TAG_ERROR, "Item %s not found", name);
	RETURN_STATUS(DAGGLE_ERROR_UNKNOWN);
}

daggle_error_code_t
resource_container_get_type(resource_container_t* resource_container,
	const char* data_type, type_info_t** out_info)
{
	ASSERT_PARAMETER(resource_container);
	
	// Get the offset to the name_hash member in type_info.
	uint64_t offset = (uint64_t)offsetof(type_info_t, name_hash);
	RETURN_STATUS(prv_name_hash_array_get_item(&resource_container->types, offset,
		data_type, (void*)out_info));
}

daggle_error_code_t
resource_container_get_node(resource_container_t* resource_container,
	const char* node_type, node_info_t** out_info)
{
	ASSERT_PARAMETER(resource_container);

	// Get the offset to the name_hash member in node_info.
	uint64_t offset = (uint64_t)offsetof(node_info_t, name_hash);
	RETURN_STATUS(prv_name_hash_array_get_item(&resource_container->nodes, offset,
		node_type, (void*)out_info));
}
