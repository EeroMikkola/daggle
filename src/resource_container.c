#include "resource_container.h"

#include "stdlib.h"
#include "string.h"
#include "utility/hash.h"
#include "utility/return_macro.h"

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
resource_container_register_node(resource_container_t* resource_container,
	const char* node_type, daggle_node_declare_fn declare)
{
	ASSERT_PARAMETER(resource_container);
	ASSERT_PARAMETER(node_type);
	ASSERT_PARAMETER(declare);

	name_with_hash_t nh
		= { .name = strdup(node_type), .hash = fnv1a_32(node_type) };

	node_info_t info = {
		.name_hash = nh,
		.declare = declare,
	};

	LOG_FMT_COND_DEBUG("Registered node %s", info.name_hash.name);

	RETURN_IF_ERROR(dynamic_array_push(&resource_container->nodes, &info));

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
resource_container_register_type(resource_container_t* resource_container,
	const char* type_name, daggle_data_clone_fn cloner,
	daggle_data_free_fn freer, daggle_data_serialize_fn serializer,
	daggle_data_deserialize_fn deserializer)
{
	ASSERT_PARAMETER(resource_container);
	ASSERT_PARAMETER(type_name);
	ASSERT_PARAMETER(cloner);
	ASSERT_PARAMETER(freer);
	ASSERT_PARAMETER(serializer);
	ASSERT_PARAMETER(deserializer);

	name_with_hash_t nh
		= { .name = strdup(type_name), .hash = fnv1a_32(type_name) };

	type_info_t info = {
		.name_hash = nh,
		.cloner = cloner,
		.freer = freer,
		.serializer = serializer,
		.deserializer = deserializer,
	};

	// LOG_FMT_COND_DEBUG("Registered type %s (%u)", info.name, info.hash);

	RETURN_IF_ERROR(dynamic_array_push(&resource_container->types, &info));

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
resource_container_get_type(resource_container_t* resource_container,
	const char* data_type, type_info_t** out_info)
{
	ASSERT_PARAMETER(resource_container);
	ASSERT_PARAMETER(data_type);
	ASSERT_OUTPUT_PARAMETER(out_info);

	const uint32_t search_hash = fnv1a_32(data_type);

	for (uint64_t i = 0; i < resource_container->types.length; ++i) {
		type_info_t* info = dynamic_array_at(&resource_container->types, i);

		ASSERT_NOT_NULL(info, "Type info is null");
		ASSERT_NOT_NULL(info->name_hash.name, "Type info name is null");

		if (info->name_hash.hash == search_hash
			&& !strcmp(data_type, info->name_hash.name)) {
			*out_info = info;
			RETURN_STATUS(DAGGLE_SUCCESS);
		}
	}

	// Not found.
	LOG_FMT(LOG_TAG_ERROR, "Type %s not found", data_type);
	RETURN_STATUS(DAGGLE_ERROR_UNKNOWN);
}

daggle_error_code_t
resource_container_get_node(resource_container_t* resource_container,
	const char* node_type, node_info_t** out_info)
{
	ASSERT_PARAMETER(resource_container);
	ASSERT_PARAMETER(node_type);
	ASSERT_OUTPUT_PARAMETER(out_info);

	const uint32_t search_hash = fnv1a_32(node_type);

	for (uint64_t i = 0; i < resource_container->nodes.length; ++i) {
		node_info_t* info = dynamic_array_at(&resource_container->nodes, i);

		ASSERT_NOT_NULL(info, "Node info is null");
		ASSERT_NOT_NULL(info->name_hash.name, "Node name is null");

		if (info->name_hash.hash == search_hash
			&& !strcmp(node_type, info->name_hash.name)) {
			*out_info = info;
			RETURN_STATUS(DAGGLE_SUCCESS);
		}
	}

	// Not found.
	RETURN_STATUS(DAGGLE_ERROR_UNKNOWN);
}
