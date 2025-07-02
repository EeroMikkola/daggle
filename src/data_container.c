#include "data_container.h"

#include "instance.h"
#include "stdio.h"
#include "stdlib.h"
#include "utility/return_macro.h"

void
data_container_init(daggle_instance_h instance, data_container_t* container)
{
	ASSERT_PARAMETER(instance);
	ASSERT_PARAMETER(container);

	container->info = NULL;
	container->data = NULL;
	container->instance = instance;
}

void
data_container_init_generate(daggle_instance_h instance,
	data_container_t* container,
	daggle_default_value_generator_fn default_value_gen)
{
	ASSERT_PARAMETER(instance);
	ASSERT_PARAMETER(default_value_gen);
	ASSERT_PARAMETER(container);

	void* default_value_data;
	const char* default_value_type;
	default_value_gen(&default_value_data, &default_value_type);

	instance_t* instance_impl = instance;
	resource_container_t* resource_container_impl
		= &instance_impl->plugin_manager.res;

	// TODO: Validate validity
	type_info_t* info;
	daggle_error_code_t error = resource_container_get_type(
		resource_container_impl, default_value_type, &info);

	container->info = info;
	container->data = default_value_data;
	container->instance = instance;
}

void
data_container_destroy(data_container_t* container)
{
	ASSERT_PARAMETER(container);

	// If the value or the type info is null -> skip.
	if (!data_container_has_value(container)) {
		return;
	}

	// Call the destructor on the data.
	daggle_data_free(container->instance, container->info->name_hash.name,
		container->data);

	// Set the contents to nullptr.
	container->data = NULL;
	container->info = NULL;
}

void
data_container_replace(data_container_t* container, type_info_t* type,
	void* data)
{
	ASSERT_PARAMETER(container);

	// If there is existing data, delete it first.
	data_container_destroy(container);

	// Overwrite the old data.
	container->data = data;
	container->info = type;
}

bool
data_container_has_value(const data_container_t* container)
{
	ASSERT_PARAMETER(container);

	return container->data && container->info;
}
