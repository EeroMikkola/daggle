#include "instance.h"
#include "resource_container.h"
#include "stdlib.h"
#include "utility/return_macro.h"

#include <daggle/daggle.h>

daggle_error_code_t
daggle_data_get_type_handlers(
	daggle_instance_h instance,
	const char* type,
	daggle_data_clone_fn* out_cloner,
	daggle_data_free_fn* out_freer,
	daggle_data_serialize_fn* out_serializer,
	daggle_data_deserialize_fn* out_deserializer)
{
	REQUIRE_PARAMETER(instance);
	REQUIRE_PARAMETER(type);

	instance_t* instance_impl = instance;
	resource_container_t* resource_container = &instance_impl->plugin_manager.res;

	type_info_t* info;
	RETURN_IF_ERROR(resource_container_get_type(resource_container, type, &info));

	if(out_cloner) {
		*out_cloner = info->cloner;
	}

	if(out_freer) {
		*out_freer = info->freer;
	}

	if(out_serializer) {
		*out_serializer = info->serializer;
	}

	if(out_deserializer) {
		*out_deserializer = info->deserializer;
	}

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_data_clone(daggle_instance_h instance, const char* type, void* data, void** out_data)
{
	REQUIRE_PARAMETER(instance);
	REQUIRE_PARAMETER(type);
	REQUIRE_PARAMETER(data);

	LOG_FMT_COND_DEBUG("Clone %s", type);

	daggle_data_clone_fn cloner;
	RETURN_IF_ERROR(daggle_data_get_type_handlers(
		instance, type, &cloner, NULL, NULL, NULL));

	cloner(instance, data, out_data);

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_data_free(daggle_instance_h instance, const char* type, void* data)
{
	REQUIRE_PARAMETER(instance);
	REQUIRE_PARAMETER(type);
	REQUIRE_PARAMETER(data);

	LOG_FMT_COND_DEBUG("Free %s", type);

	daggle_data_free_fn destructor;
	RETURN_IF_ERROR(daggle_data_get_type_handlers(
		instance, type, NULL, &destructor, NULL, NULL));

	destructor(instance, data);

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_data_serialize(
	daggle_instance_h instance,
	const char* type,
	const void* data,
	unsigned char** out_bin,
	uint64_t* out_len)
{
	REQUIRE_PARAMETER(instance);
	REQUIRE_PARAMETER(type);
	REQUIRE_PARAMETER(data);
	REQUIRE_OUTPUT_PARAMETER(out_bin);
	REQUIRE_OUTPUT_PARAMETER(out_len);

	LOG_FMT_COND_DEBUG("Serialize %s", type);

	daggle_data_serialize_fn serializer;
	RETURN_IF_ERROR(daggle_data_get_type_handlers(
		instance, type, NULL, NULL, &serializer, NULL));

	serializer(instance, data, out_bin, out_len);

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_data_deserialize(
	daggle_instance_h instance,
	const char* type,
	const unsigned char* bin,
	uint64_t len,
	void** out_data)
{
	REQUIRE_PARAMETER(instance);
	REQUIRE_PARAMETER(type);
	REQUIRE_PARAMETER(bin);
	REQUIRE_OUTPUT_PARAMETER(out_data);

	LOG_FMT_COND_DEBUG("Deserialize %s", type);

	daggle_data_deserialize_fn deserializer;
	RETURN_IF_ERROR(daggle_data_get_type_handlers(
		instance, type, NULL, NULL, NULL, &deserializer));

	deserializer(instance, bin, len, out_data);

	RETURN_STATUS(DAGGLE_SUCCESS);
}