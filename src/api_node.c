#include "graph.h"
#include "instance.h"
#include "node.h"
#include "stdlib.h"
#include "string.h"
#include "utility/return_macro.h"

#include <daggle/daggle.h>

daggle_error_code_t
prv_node_declare_port(daggle_port_variant_t variant, daggle_node_h node,
	const char* port_name, daggle_input_variant_t input_variant,
	daggle_default_value_generator_fn default_value_gen)
{
	ASSERT_PARAMETER(node);
	ASSERT_PARAMETER(port_name);

	// Cast from opaque back to the real type.
	node_t* node_impl = node;

	port_t* preexisting_port;
	daggle_node_get_port_by_name(node_impl, port_name,
		(daggle_port_h*)&preexisting_port);

	// If the port already exists -> flag as declared.
	if (preexisting_port) {
		preexisting_port->declared = true;
		RETURN_STATUS(DAGGLE_SUCCESS);
	}

	// The declared port does not pre-exist.
	// Create an input and add to the ports.

	// Extract instance
	graph_t* graph = node_impl->graph;
	instance_t* instance = graph->instance;

	// Create the data container of the port.
	data_container_t default_value_cnt;
	if (default_value_gen) {
		data_container_init_generate(instance, &default_value_cnt,
			default_value_gen);
	} else {
		data_container_init(instance, &default_value_cnt);
	}

	port_t new_port;
	port_init(node_impl, port_name, variant, &new_port);
	new_port.value = default_value_cnt;

	if (variant == DAGGLE_PORT_INPUT) {
		new_port.variant.input.variant = input_variant;
	}

	// Set the created port as declared.
	new_port.declared = true;

	RETURN_STATUS(dynamic_array_push(&node_impl->ports, &new_port));
}

daggle_error_code_t
daggle_node_declare_input(daggle_node_h node, const char* port_name,
	daggle_input_variant_t variant,
	daggle_default_value_generator_fn default_value_gen)
{
	REQUIRE_PARAMETER(node);
	REQUIRE_PARAMETER(port_name);
	REQUIRE_PARAMETER(default_value_gen);

	RETURN_STATUS(prv_node_declare_port(DAGGLE_PORT_INPUT, node, port_name,
		variant, default_value_gen));
}

daggle_error_code_t
daggle_node_declare_parameter(daggle_node_h node, const char* port_name,
	daggle_default_value_generator_fn default_value_gen)
{
	REQUIRE_PARAMETER(node);
	REQUIRE_PARAMETER(port_name);
	REQUIRE_PARAMETER(default_value_gen);

	RETURN_STATUS(prv_node_declare_port(DAGGLE_PORT_PARAMETER, node, port_name,
		false, default_value_gen));
}

daggle_error_code_t
daggle_node_declare_output(daggle_node_h node, const char* port_name)
{
	REQUIRE_PARAMETER(node);
	REQUIRE_PARAMETER(port_name);

	RETURN_STATUS(prv_node_declare_port(DAGGLE_PORT_OUTPUT, node, port_name,
		false, NULL));
}

daggle_error_code_t
daggle_node_declare_task(daggle_node_h node, daggle_node_task_fn task)
{
	REQUIRE_PARAMETER(node);
	REQUIRE_PARAMETER(task);

	node_t* node_impl = node;
	node_impl->instance_task = task;

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_node_declare_context(daggle_node_h node, void* context,
	daggle_node_context_free_fn destructor)
{
	REQUIRE_PARAMETER(node);

	node_t* internal_node = node;

	if (internal_node->custom_context_destructor) {
		internal_node->custom_context_destructor(internal_node->custom_context);
	}

	internal_node->custom_context = context;
	internal_node->custom_context_destructor = destructor;

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_node_get_type(daggle_node_h node, const char** out_type)
{
	REQUIRE_PARAMETER(node);
	REQUIRE_OUTPUT_PARAMETER(out_type);

	node_t* internal_node = node;

	*out_type = internal_node->info->name_hash.name;

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_node_get_daggle(daggle_node_h node, daggle_instance_h* out_daggle)
{
	REQUIRE_PARAMETER(node);
	REQUIRE_OUTPUT_PARAMETER(out_daggle);

	daggle_graph_h graph;
	daggle_node_get_graph(node, &graph);

	RETURN_STATUS(daggle_graph_get_daggle(graph, out_daggle));
}

daggle_error_code_t
daggle_node_get_graph(daggle_node_h node, daggle_graph_h* out_graph)
{
	REQUIRE_PARAMETER(node);
	REQUIRE_OUTPUT_PARAMETER(out_graph);

	node_t* node_impl = node;
	graph_t* graph = node_impl->graph;

	*out_graph = graph;

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_node_get_port_by_name(daggle_node_h node, const char* port_name,
	daggle_port_h* out_port)
{
	REQUIRE_PARAMETER(node);
	REQUIRE_PARAMETER(port_name);
	REQUIRE_OUTPUT_PARAMETER(out_port);

	port_t* port = node_get_port_by_name(node, port_name);

	*out_port = port;
	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_node_get_port_by_index(daggle_node_h node, uint64_t index,
	daggle_port_h* out_port)
{
	REQUIRE_PARAMETER(node);
	REQUIRE_OUTPUT_PARAMETER(out_port);

	node_t* node_impl = node;
	if (index >= node_impl->ports.length) {
		*out_port = NULL;
		RETURN_STATUS(DAGGLE_SUCCESS);
	}

	port_t* port = node_get_port_by_index(node_impl, index);

	*out_port = port;
	RETURN_STATUS(DAGGLE_SUCCESS);
}