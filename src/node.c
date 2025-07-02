#include "node.h"

#include "data_container.h"
#include "graph.h"
#include "instance.h"
#include "stdatomic.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "utility/dynamic_array.h"
#include "utility/hash.h"
#include "utility/return_macro.h"

daggle_error_code_t
node_create(daggle_graph_h graph, const char* node_type, node_t** out_node)
{
	ASSERT_PARAMETER(graph);
	ASSERT_PARAMETER(node_type);
	ASSERT_OUTPUT_PARAMETER(out_node);

	graph_t* graph_impl = graph;

	// Get the node info pointer.
	node_info_t* info;
	RETURN_IF_ERROR(resource_container_get_node(
		&graph_impl->instance->plugin_manager.res, node_type, &info));

	// Allocate a new node instance
	node_t* node = malloc(sizeof *node);
	REQUIRE_ALLOCATION_DAGGLE_SUCCESSFUL(node);

	// Create a port array.
	daggle_error_code_t error
		= dynamic_array_init(0, sizeof(port_t), &node->ports);
	if (error != DAGGLE_SUCCESS) {
		free(node);
		RETURN_STATUS(error);
	}

	node->instance_task = NULL;
	node->info = info;

	node->graph = graph_impl;

	node->custom_context = NULL;
	node->custom_context_destructor = NULL;

	node_compute_declarations(node);

	*out_node = node;
	RETURN_STATUS(DAGGLE_SUCCESS);
}

void
node_free(node_t* node)
{
	ASSERT_PARAMETER(node);

	if (node->custom_context_destructor) {
		node->custom_context_destructor(node->custom_context);
	}

	for (uint64_t i = 0; i < node->ports.length; ++i) {
		port_t* port = dynamic_array_at(&node->ports, i);

		// Only inputs and outputs have edges. Skip the parameter.
		if (port->port_variant != DAGGLE_PORT_PARAMETER) {
			continue;
		}

		port_destroy(port);
	}

	dynamic_array_destroy(&node->ports);

	free(node);
}

port_t*
node_get_port_by_name(node_t* node, const char* port_name)
{
	ASSERT_PARAMETER(node);
	ASSERT_PARAMETER(port_name);

	uint32_t search_hash = fnv1a_32(port_name);

	for (uint64_t i = 0; i < node->ports.length; ++i) {
		port_t* item = dynamic_array_at(&node->ports, i);

		if (item->name_hash.hash == search_hash
			&& !strcmp(port_name, item->name_hash.name)) {
			return item;
		}
	}

	return NULL;
}

port_t*
node_get_port_by_index(node_t* node, uint64_t index)
{
	ASSERT_PARAMETER(node);

	ASSERT_TRUE(index < node->ports.length, "Port index out of bounds");

	return dynamic_array_at(&node->ports, index);
}

void
node_compute_declarations(node_t* node)
{
	ASSERT_PARAMETER(node);

	if (node->custom_context_destructor) {
		node->custom_context_destructor(node->custom_context);
	}

	node->custom_context = NULL;
	node->custom_context_destructor = NULL;

	// Reset declaration state flags to undeclared.
	for (uint64_t i = 0; i < node->ports.length; ++i) {
		port_t* port = dynamic_array_at(&node->ports, i);
		port->declared = false;
	}

	// Get the node declarator function.
	daggle_node_declare_fn declare_fn = node->info->declare;

	// Declaration functions should set the flag on.
	declare_fn(node);

	// Remove undeclared ports.
	for (uint64_t i = 0; i < node->ports.length; ++i) {
		port_t* port = dynamic_array_at(&node->ports, i);

		if (!port->declared) {
			port_destroy(port);
			dynamic_array_remove(&node->ports, i);
		}
	}

	// If custom context was not set, implicitly use node as the context.
	if (!node->custom_context_destructor && !node->custom_context) {
		daggle_node_declare_context(node, node, NULL);
	}
}
