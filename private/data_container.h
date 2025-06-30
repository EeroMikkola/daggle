#pragma once

#include "resource_container.h"
#include "stdint.h"

#include <daggle/daggle.h>

typedef struct data_container_s {
	void* data;
	type_info_t* info;
	daggle_instance_h instance;
} data_container_t;

void
data_container_init(daggle_instance_h instance, data_container_t* container);

void
data_container_init_generate(daggle_instance_h instance,
	data_container_t* container,
	daggle_default_value_generator_fn default_value_gen);

void
data_container_destroy(data_container_t* container);

void
data_container_replace(data_container_t* container,
	type_info_t* type,
	void* data);

bool
data_container_has_value(const data_container_t* container);
