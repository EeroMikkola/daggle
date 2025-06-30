#pragma once

#include "data_container.h"
#include "resource_container.h"
#include "utility/dynamic_array.h"

#include <daggle/daggle.h>

typedef struct port_variant_output_s {
	dynamic_array_t links;
} port_variant_output_t;

typedef struct port_variant_input_s {
	daggle_port_h link;
	daggle_input_variant_t variant;
} port_variant_input_t;

typedef struct port_variant_parameter_s {
} port_variant_parameter_t;

typedef struct port_s {
	name_with_hash_t name_hash;

	// Stores the data and type info
	data_container_t value;

	// Is the port input, output or parameter.
	daggle_port_variant_t port_variant;

	// Variant-specific fields.
	union {
		port_variant_input_t input;
		port_variant_output_t output;
		port_variant_parameter_t param;
	} variant;

	// Pointer to the node which the port belongs to.
	daggle_node_h owner;

	// Used to determine if the port does not get declared in node declare.
	bool declared;
} port_t;

void
port_init(daggle_node_h node,
	const char* name,
	data_container_t data,
	daggle_port_variant_t variant,
	port_t* port);

void
port_destroy(port_t* port);
