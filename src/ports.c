#include "ports.h"

#include "node.h"
#include "resource_container.h"
#include "stdlib.h"
#include "string.h"
#include "utility/hash.h"
#include "utility/return_macro.h"

void
port_destroy(port_t* port)
{
	ASSERT_PARAMETER(port);

	free((char*)port->name_hash.name);

	if (port->port_variant != DAGGLE_PORT_PARAMETER) {
		daggle_port_disconnect(port);
	}

	if (port->port_variant == DAGGLE_PORT_OUTPUT) {
		dynamic_array_destroy(&port->variant.output.links);
	}

	data_container_destroy(&port->value);
}

void
port_init(daggle_node_h node, const char* port_name,
	daggle_port_variant_t variant, port_t* out_port)
{
	ASSERT_PARAMETER(node);
	ASSERT_PARAMETER(port_name);
	ASSERT_PARAMETER(out_port);

	port_t port = {
		.name_hash = { 
			.name = strdup(port_name), 
			.hash = fnv1a_32(port_name) 
		},
		.owner = node,
		.port_variant = variant
	};

	daggle_instance_h instance;
	daggle_graph_get_daggle(((node_t*)node)->graph, &instance);
	data_container_init(instance, &port.value);

	if (variant == DAGGLE_PORT_INPUT) {
		port.variant.input.link = NULL;
		port.variant.input.behavior = DAGGLE_INPUT_BEHAVIOR_REFERENCE;
	} else if (variant == DAGGLE_PORT_OUTPUT) {
		dynamic_array_init(0, sizeof(port_t*), &port.variant.output.links);
	}

	*out_port = port;
}
