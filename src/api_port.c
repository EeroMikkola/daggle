#include "graph.h"
#include "instance.h"
#include "node.h"
#include "ports.h"
#include "resource_container.h"
#include "utility/log_macro.h"
#include "utility/return_macro.h"

#include <daggle/daggle.h>

// Writes the input port address to out_target and output to out_source.
daggle_error_code_t
prv_sort_ports_to_source_and_target(port_t* port_a, port_t* port_b,
	port_t** out_source, port_t** out_target)
{
	ASSERT_PARAMETER(port_a);
	ASSERT_PARAMETER(port_b);

	ASSERT_OUTPUT_PARAMETER(out_source);
	ASSERT_OUTPUT_PARAMETER(out_target);

	if (port_a->port_variant == DAGGLE_PORT_OUTPUT
		&& port_b->port_variant == DAGGLE_PORT_INPUT) {
		*out_source = port_a;
		*out_target = port_b;

		RETURN_STATUS(DAGGLE_SUCCESS);
	} else if (port_a->port_variant == DAGGLE_PORT_INPUT
		&& port_b->port_variant == DAGGLE_PORT_OUTPUT) {
		*out_source = port_b;
		*out_target = port_a;

		RETURN_STATUS(DAGGLE_SUCCESS);
	}

	RETURN_STATUS(DAGGLE_ERROR_INCORRECT_PORT_VARIANT);
}

daggle_error_code_t
daggle_port_connect(daggle_port_h port_a, daggle_port_h port_b)
{
	REQUIRE_PARAMETER(port_a);
	REQUIRE_PARAMETER(port_b);

	port_t* source;
	port_t* target;
	RETURN_IF_ERROR(
		prv_sort_ports_to_source_and_target(port_a, port_b, &source, &target));

	node_t* source_parent = source->owner;
	node_t* target_parent = target->owner;
	LOG_FMT_COND_DEBUG("Connect %s of %s to %s of %s", source->name_hash.name,
		source_parent->info->name_hash.name, target->name_hash.name,
		target_parent->info->name_hash.name);

	// If there is a pre-existing connection, remove it.
	if (target->variant.input.link) {
		RETURN_IF_ERROR(daggle_port_disconnect(target));
	}

	// Set connections
	target->variant.input.link = source;

	// Push a pointer to the target node to the link array.
	// Push copies stride bytes from the address data points to.
	// To push a pointer, the address of the pointer variable must be provided.
	// Providing only the pointer would copy stride-bytes from the instance.
	// For port, that would copy the port name field.
	RETURN_STATUS(dynamic_array_push(&source->variant.output.links, &target));
}

// disconnection_index is a pointer to enable optional usage.
daggle_error_code_t
prv_port_disconnect(port_t* port, const uint64_t* disconnection_index)
{
	ASSERT_PARAMETER(port);

	node_t* port_owner = port->owner;
	LOG_FMT_COND_DEBUG("Disconnect %s %s", port_owner->info->name_hash.name,
		port->name_hash.name);

	if (port->port_variant == DAGGLE_PORT_PARAMETER) {
		// Parameters can't have edges, return an error.
		RETURN_STATUS(DAGGLE_ERROR_INCORRECT_PORT_VARIANT);
	} else if (port->port_variant == DAGGLE_PORT_OUTPUT) {
		dynamic_array_t* links = &port->variant.output.links;
		if (disconnection_index) {
			port_t** element = dynamic_array_at(links, *disconnection_index);
			port_t* item = *element;

			// The outgoing edge index was specified. Disconnect that
			// edge using the input port of that.
			// At provides a pointer to the data. The link array stores pointers
			// towards other ports, therefore the return type is port**.
			RETURN_STATUS(prv_port_disconnect(item, disconnection_index));
		} else {
			// An outgoing edge was not sepcified. Disconnect every conencted
			// edge. Due to the way dynamic array was implemented, it makes more
			// sense to disconnect the ports in a reverse order.
			for (int i = links->length - 1; i >= 0; --i) {
				port_t** element = dynamic_array_at(links, i);
				port_t* item = *element;

				// We're using an int here to prevent the uint64_t from
				// overflowing. However, the function uses a pointer to a
				// uint64_t instead of an int, which is why i is cast into the
				// index variable.
				const uint64_t index = i;
				RETURN_IF_ERROR(prv_port_disconnect(item, &index));
			}

			RETURN_STATUS(DAGGLE_SUCCESS);
		}
	} else if (port->port_variant == DAGGLE_PORT_INPUT) {
		// Get the output (source) port of the edge.
		port_t* link = port->variant.input.link;

		// Return if the port does not have an edge to disconnect.
		if (!link) {
			RETURN_STATUS(DAGGLE_SUCCESS);
		}

		// Get the list of connected input (target) ports of the source port.
		dynamic_array_t* links = &link->variant.output.links;

		if (disconnection_index) {
			// Remove link from the target.
			port->variant.input.link = NULL;

			// Remove link from the source.
			dynamic_array_remove(links, *disconnection_index);

			RETURN_STATUS(DAGGLE_SUCCESS);
		} else {
			// Get the index of the input port in the edge list of the
			// connected output port. If it is found (it definitely should),
			// call this function with the index of the port.
			for (uint64_t i = 0; i < links->length; ++i) {
				port_t** element = dynamic_array_at(links, i);
				port_t* item = *element;

				if (item != link) {
					continue;
				}

				RETURN_STATUS(prv_port_disconnect(item, &i));
			}

			// Port was not found, should not happen.
			RETURN_STATUS(DAGGLE_ERROR_UNKNOWN);
		}
	}

	// It shouldn't be possible to reach this.
	RETURN_STATUS(DAGGLE_ERROR_UNKNOWN);
}

daggle_error_code_t
daggle_port_disconnect(daggle_port_h port)
{
	REQUIRE_PARAMETER(port);

	RETURN_STATUS(prv_port_disconnect(port, NULL));
}

daggle_error_code_t
daggle_port_get_connected_port_by_index(daggle_port_h port, uint64_t index,
	daggle_port_h* out_port)
{
	REQUIRE_PARAMETER(port);
	REQUIRE_OUTPUT_PARAMETER(out_port);

	port_t* internal_port = port;

	switch (internal_port->port_variant) {
	case DAGGLE_PORT_PARAMETER:
		RETURN_STATUS(DAGGLE_ERROR_INCORRECT_PORT_VARIANT);
	case DAGGLE_PORT_INPUT:
		if (index != 0) {
			*out_port = NULL;
		} else {
			*out_port = internal_port->variant.input.link;
		}

		RETURN_STATUS(DAGGLE_SUCCESS);
	case DAGGLE_PORT_OUTPUT:
		if (index >= internal_port->variant.output.links.length) {
			*out_port = NULL;
		} else {
			*out_port
				= dynamic_array_at(&internal_port->variant.output.links, index);
		}

		RETURN_STATUS(DAGGLE_SUCCESS);
	default:
		RETURN_STATUS(DAGGLE_ERROR_UNKNOWN);
	}
}

daggle_error_code_t
daggle_port_get_name(const daggle_port_h port, const char** out_name)
{
	REQUIRE_PARAMETER(port);
	REQUIRE_OUTPUT_PARAMETER(out_name);

	const port_t* port_impl = port;
	const char* port_name = port_impl->name_hash.name;

	*out_name = port_name;

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_port_get_node(const daggle_port_h port, daggle_node_h* out_node)
{
	REQUIRE_PARAMETER(port);
	REQUIRE_OUTPUT_PARAMETER(out_node);

	const port_t* port_impl = port;
	node_t* port_owner = port_impl->owner;

	*out_node = port_owner;

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_port_get_variant(const daggle_port_h port,
	daggle_port_variant_t* out_variant)
{
	REQUIRE_PARAMETER(port);
	REQUIRE_OUTPUT_PARAMETER(out_variant);

	const port_t* port_impl = port;
	daggle_port_variant_t port_variant = port_impl->port_variant;

	*out_variant = port_variant;

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_port_get_value_data_type(const daggle_port_h port,
	const char** out_data_type)
{
	REQUIRE_PARAMETER(port);
	REQUIRE_OUTPUT_PARAMETER(out_data_type);

	const port_t* port_impl = port;

	const data_container_t* data_location = &port_impl->value;

	// If the port is input and the input has an edge with data,
	// return the type of the edge-provided data.
	if (port_impl->port_variant == DAGGLE_PORT_INPUT
		&& port_impl->variant.input.link != NULL) {
		port_t* link = port_impl->variant.input.link;
		data_location = &link->value;
	}

	if (!data_container_has_value(data_location)) {
		*out_data_type = "";
		RETURN_STATUS(DAGGLE_SUCCESS);
	}

	ASSERT_NOT_NULL(data_location->info->name_hash.name,
		"The type of the value of the port is null");

	*out_data_type = data_location->info->name_hash.name;

	RETURN_STATUS(DAGGLE_SUCCESS);
}

void
prv_port_get_value_as_reference(port_t* port, void** out_data)
{
	ASSERT_PARAMETER(port);
	ASSERT_OUTPUT_PARAMETER(out_data);

	// If the port does not have data, return NULL.
	if (!data_container_has_value(&port->value)) {
		*out_data = NULL;
		return;
	}

	// Parameter ports return an immutable pointer to the stored data.
	*out_data = port->value.data;
}

void
prv_port_get_value_as_copy(port_t* port, void** out_data)
{
	ASSERT_PARAMETER(port);
	ASSERT_OUTPUT_PARAMETER(out_data);

	// If the port does not have data, return NULL.
	if (!data_container_has_value(&port->value)) {
		*out_data = NULL;
		return;
	}

	port_t* port_impl = port;

	daggle_data_clone(port->value.instance, port->value.info->name_hash.name,
		port->value.data, out_data);
}

void
prv_input_get_value(port_t* port, void** out_data)
{
	ASSERT_PARAMETER(port);
	ASSERT_OUTPUT_PARAMETER(out_data);

	// Get pointer to the port, which the provided input is connected to.
	// This is NULL when the input does not have an edge.
	port_t* link = port->variant.input.link;

	node_t* port_owner = port->owner;
	graph_t* graph = port_owner->graph;
	bool is_locked = graph->locked;

	// Determine which port to use as the source.
	port_t* target_port = link ? link : port;

	switch (port->variant.input.behavior) {
	case DAGGLE_INPUT_BEHAVIOR_REFERENCE:
		prv_port_get_value_as_reference(target_port, out_data);
		break;
	case DAGGLE_INPUT_BEHAVIOR_ACQUIRE:
		// Acquire is available only if port is linked, and it is the only link from the output
		if (link && link->variant.output.links.length == 1) {
			*out_data = link->value.data;
			link->value.data = NULL;
			link->value.info = NULL;
		} 
		// Otherwise use the cloning behavior.
	case DAGGLE_INPUT_BEHAVIOR_CLONE:
		prv_port_get_value_as_copy(target_port, out_data);
		break;
	}
}

// TODO: Make separate function for input, one for prv_input_get_value, one for
// prv_port_get_value_as_reference (read externally), OR, switch between them
// based on node execution
daggle_error_code_t
daggle_port_get_value(const daggle_port_h port, void** out_data)
{
	REQUIRE_PARAMETER(port);
	REQUIRE_OUTPUT_PARAMETER(out_data);

	port_t* port_impl = port;

	switch (port_impl->port_variant) {
	case DAGGLE_PORT_INPUT:
		prv_input_get_value(port_impl, out_data);
		break;
	case DAGGLE_PORT_PARAMETER:
	case DAGGLE_PORT_OUTPUT:
		prv_port_get_value_as_reference(port_impl, out_data);
		break;
	default:
		RETURN_STATUS(DAGGLE_ERROR_UNKNOWN);
	}

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_port_set_value(const daggle_port_h port, const char* data_type,
	void* data)
{
	REQUIRE_PARAMETER(port);
	REQUIRE_PARAMETER(data_type);

	// TODO: Critical! Return error if node is currently being declared.
	// If a port is being set, it will run compute declarations twice
	// (potentially loops infinitely), breaking it.

	// TODO: Ensure data type is allowed for the port.
	// Which means implementing some sort of port data type constraint system,
	// and ideally data conversions.

	port_t* port_impl = port;

	node_t* port_owner = port_impl->owner;
	graph_t* graph = port_owner->graph;
	instance_t* instance = graph->instance;

	bool is_locked = graph->locked;

	bool is_port_input = port_impl->port_variant == DAGGLE_PORT_INPUT;
	bool is_port_output = port_impl->port_variant == DAGGLE_PORT_OUTPUT;
	bool is_port_param = port_impl->port_variant == DAGGLE_PORT_PARAMETER;

	bool is_port_linked_input
		= is_port_input && port_impl->variant.input.link != NULL;

	bool is_port_unlinked_input
		= is_port_input && port_impl->variant.input.link == NULL;

	bool is_subject_to_locking = is_port_param;

	// Node is required to be unlocked to be modified.
	if (is_locked && is_subject_to_locking) {
		RETURN_STATUS(DAGGLE_ERROR_OBJECT_LOCKED);
	}

	// If setting output outside of a node.
	if (!is_locked && is_port_output) {
		ASSERT_TRUE(false,
			"Changing output port while not locked; allow "
			"if intentional "
			"(i.e. cache)");
		RETURN_STATUS(DAGGLE_ERROR_OBJECT_LOCKED);
	}

	// If setting linked value outside of a node.
	if (!is_locked && is_port_linked_input) {
		LOG(LOG_TAG_WARN, "Setting linked input port outside of node!");
	}

	type_info_t* info;
	RETURN_IF_ERROR(resource_container_get_type(&instance->plugin_manager.res,
		data_type, &info));

	data_container_replace(&port_impl->value, info, data);

	if (is_port_param) {
		// A parameter was changed, should invoke node redeclaration.
		node_compute_declarations(port_owner);
	}

	RETURN_STATUS(DAGGLE_SUCCESS);
}
