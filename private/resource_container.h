#pragma once

#include "utility/dynamic_array.h"

#include <daggle/daggle.h>

typedef struct name_with_hash_s {
	const char* name;
	uint32_t hash;
} name_with_hash_t;

typedef struct node_info_s {
	name_with_hash_t name_hash;
	daggle_node_declare_fn declare;
} node_info_t;

// TODO: conversion functions T -> U and K<T> -> K<U>
// T -> U may be use empty base: _<T> -> _<U>
// Converter has source and target pointers, source type and target type,
// the converter function, instance context.

typedef struct type_info_s {
	name_with_hash_t name_hash;

	daggle_data_clone_fn cloner;
	daggle_data_free_fn freer;

	daggle_data_serialize_fn serializer;
	daggle_data_deserialize_fn deserializer;
} type_info_t;

typedef struct resource_container_s {
	dynamic_array_t nodes; // node_info_t[]
	dynamic_array_t types; // type_info_t[]
} resource_container_t;

void
resource_container_init(resource_container_t* resource_container);

void
resource_container_destroy(resource_container_t* resource_container);

daggle_error_code_t
resource_container_get_type(resource_container_t* resource_container,
	const char* type, type_info_t** out_info);

daggle_error_code_t
resource_container_get_node(resource_container_t* resource_container,
	const char* type, node_info_t** out_info);

daggle_error_code_t
daggle_plugin_register_node(daggle_instance_h instance,
	const char* type, daggle_node_declare_fn declare);

daggle_error_code_t
daggle_plugin_register_type(daggle_instance_h instance,
	const char* type, daggle_data_clone_fn cloner, daggle_data_free_fn freer,
	daggle_data_serialize_fn serializer,
	daggle_data_deserialize_fn deserializer);