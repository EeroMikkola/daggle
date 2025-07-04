#include "serialization.h"

#include "graph.h"
#include "memory.h"
#include "node.h"
#include "ports.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "utility/return_macro.h"

port_variant_1_t
prv_port_variant_daggle_to_1(daggle_port_variant_t variant)
{
	switch (variant) {
	default:
		LOG_FMT(LOG_TAG_ERROR, "Unknown port variant %u", variant);
	case DAGGLE_PORT_INPUT:
		return INPUT;
	case DAGGLE_PORT_OUTPUT:
		return OUTPUT;
	case DAGGLE_PORT_PARAMETER:
		return PARAMETER;
	}
}

daggle_port_variant_t
prv_port_variant_1_to_daggle(port_variant_1_t variant)
{
	switch (variant) {
	default:
		LOG_FMT(LOG_TAG_ERROR, "Unknown variant %u", variant);
	case INPUT:
		return DAGGLE_PORT_INPUT;
	case OUTPUT:
		return DAGGLE_PORT_OUTPUT;
	case PARAMETER:
		return DAGGLE_PORT_PARAMETER;
	}
}

input_behavior_1_t
prv_input_behavior_daggle_to_1(daggle_input_behavior_t variant)
{
	switch (variant) {
	default:
		LOG_FMT(LOG_TAG_ERROR, "Unknown input variant %u", variant);
	case DAGGLE_INPUT_BEHAVIOR_REFERENCE:
		return REFERENCE;
	case DAGGLE_INPUT_BEHAVIOR_CLONE:
		return CLONE;
	case DAGGLE_INPUT_BEHAVIOR_ACQUIRE:
		return ACQUIRE;
	}
}

daggle_input_behavior_t
prv_input_behavior_1_to_daggle(input_behavior_1_t variant)
{
	switch (variant) {
	default:
		LOG_FMT(LOG_TAG_ERROR, "Unknown input variant %u", variant);
	case REFERENCE:
		return DAGGLE_INPUT_BEHAVIOR_REFERENCE;
	case CLONE:
		return DAGGLE_INPUT_BEHAVIOR_CLONE;
	case ACQUIRE:
		return DAGGLE_INPUT_BEHAVIOR_ACQUIRE;
	}
}

// Find the flattened index of a port.
uint64_t
prv_get_port_flat_index(const graph_t* graph, const port_t* port)
{
	uint64_t counter = 0;

	for (int i = 0; i < graph->nodes.length; i++) {
		node_t** node_element = dynamic_array_at(&graph->nodes, i);
		node_t* node = *node_element;

		for (int j = 0; j < node->ports.length; j++) {
			port_t* port_element = dynamic_array_at(&node->ports, j);

			if (port_element == port) {
				return counter;
			}

			counter++;
		}
	}

	printf("Port counting failed");
	return UINT32_MAX;
}

// Find the node and port indices of the Nth port. Essentially reverse of the
// above.
void
prv_get_flat_port_indices(uint64_t edge_ptidx, uint64_t num_nodes,
	const node_entry_1_t* nodes, uint64_t* out_node_idx, uint64_t* out_port_idx)
{
	uint64_t counter = 0;
	for (int i = 0; i < num_nodes; i++) {
		const node_entry_1_t* node_entry = nodes + i;
		for (int j = 0; j < node_entry->num_ports; j++) {
			if (counter == edge_ptidx) {
				*out_port_idx = j;
				*out_node_idx = i;
				return;
			}

			counter++;
		}
	}

	printf("Port counting failed");
}

void
prv_offset_write(void* target, const void* source, uint64_t stride,
	uint64_t* ref_offset)
{
	memcpy(target + *ref_offset, source, stride);
	*ref_offset += stride;
}

void
prv_write_arrays_to_bin(const dynamic_array_t* node_entries,
	const dynamic_array_t* port_entries, const dynamic_array_t* string_buffer,
	const dynamic_array_t* data_buffer, unsigned char** out_bin,
	uint64_t* out_len)
{
	uint64_t total_size = 0;

	total_size += sizeof(graph_1_t);
	total_size += node_entries->stride * node_entries->length;
	total_size += node_entries->stride * port_entries->length;
	total_size += string_buffer->stride * string_buffer->length;
	total_size += data_buffer->stride * data_buffer->length;

	unsigned char* bin = malloc(total_size);
	uint64_t offset = 0;

	graph_1_t graph_bin = { .version = 1,
		.num_nodes = node_entries->length,
		.num_ports = port_entries->length,
		.text_len = string_buffer->length,
		.data_len = data_buffer->length };

	prv_offset_write(bin, &graph_bin, sizeof(graph_1_t), &offset);

	prv_offset_write(bin, node_entries->data,
		node_entries->stride * node_entries->length, &offset);

	prv_offset_write(bin, port_entries->data,
		port_entries->stride * port_entries->length, &offset);

	prv_offset_write(bin, string_buffer->data,
		string_buffer->stride * string_buffer->length, &offset);

	prv_offset_write(bin, data_buffer->data,
		data_buffer->stride * data_buffer->length, &offset);

	*out_bin = bin;
	*out_len = total_size;
}

void
prv_append_string_buffer(dynamic_array_t* string_buffer, const char* source,
	uint64_t* out_index)
{
	// Get the start offset of the string.
	uint64_t start_offset = string_buffer->length;

	// Get the length of the source string plus its null terminator.
	uint64_t string_length = strlen(source) + 1;

	// Add empty space the size of the string into the string buffer.
	dynamic_array_resize(string_buffer, string_buffer->length + string_length);
	string_buffer->length = string_buffer->capacity;

	// Copy the string into the string buffer.
	memcpy(string_buffer->data + start_offset, source, string_length);

	// Return the start offset of the string.
	*out_index = start_offset;
}

void
prv_append_data_buffer(dynamic_array_t* data_buffer,
	dynamic_array_t* string_buffer, const char* data_type,
	unsigned char* data_bin, uint64_t data_len, uint64_t* out_dtoff)
{
	// Write data type to string buffer.
	uint64_t type_stoff = string_buffer->length;
	prv_append_string_buffer(string_buffer, data_type, &type_stoff);

	// Get the start offset of the data entry.
	uint64_t start_offset = data_buffer->length;

	// Calculate the size of the data entry struct.
	uint64_t data_entry_size = sizeof(data_entry_1_t) + data_len;

	// Add empty space the size of the data entry into the data buffer.
	dynamic_array_resize(data_buffer, data_buffer->length + data_entry_size);
	data_buffer->length = data_buffer->capacity;

	// Get the location of the data entry.
	data_entry_1_t* entry = data_buffer->data + start_offset;

	// Copy data to the data entry fields
	memcpy(&entry->type_stoff, &type_stoff, sizeof(entry->type_stoff));
	memcpy(&entry->size, &data_len, sizeof(entry->size));
	memcpy(&entry->bytes, data_bin, data_len);

	// Return the offset to the data entry.
	*out_dtoff = start_offset;
}

void
prv_port_serialize_and_push(port_t* port, graph_t* graph,
	dynamic_array_t* port_entries, dynamic_array_t* string_buffer,
	dynamic_array_t* data_buffer)
{
	port_entry_1_t port_entry;

	// Initialize port entry
	port_entry.name_stoff = 0;
	port_entry.edge_ptidx = UINT64_MAX; // Unset is MAX
	port_entry.data_dtoff = UINT64_MAX;

	// Write port name to string buffer
	// and store character offset to name_stoff.
	prv_append_string_buffer(string_buffer, port->name_hash.name,
		&port_entry.name_stoff);

	port_entry.port_variant = prv_port_variant_daggle_to_1(port->port_variant);

	// For input ports, set the input variant, and link if it has one
	if (port->port_variant == DAGGLE_PORT_INPUT) {
		port_entry.port_specific.input
			= prv_input_behavior_daggle_to_1(port->variant.input.behavior);

		if (port->variant.input.link) {
			port_entry.edge_ptidx
				= prv_get_port_flat_index(graph, port->variant.input.link);
		}
	}

	if (data_container_has_value(&port->value)) {
		unsigned char* data_bin;
		uint64_t data_len;

		const char* data_type = port->value.info->name_hash.name;

		daggle_data_serialize(graph->instance, data_type, port->value.data,
			&data_bin, &data_len);

		prv_append_data_buffer(data_buffer, string_buffer, data_type, data_bin,
			data_len, &port_entry.data_dtoff);
	}

	dynamic_array_push(port_entries, &port_entry);
}

void
prv_node_serialize_and_push(node_t* node, graph_t* graph,
	dynamic_array_t* node_entries, dynamic_array_t* port_entries,
	dynamic_array_t* string_buffer, dynamic_array_t* data_buffer)
{
	node_entry_1_t entry;

	// Push node id (currently unused) into strings, store offset in node entry.
	const char dummy_id[] = "dummy_id";
	prv_append_string_buffer(string_buffer, dummy_id, &entry.name_stoff);

	// Push node type into strings, store offset in node entry.
	prv_append_string_buffer(string_buffer, node->info->name_hash.name,
		&entry.type_stoff);

	// Set port-related fields.
	entry.num_ports = node->ports.length;
	entry.first_port_ptidx = port_entries->length;

	// Push the serialized node entry into the node entries.
	dynamic_array_push(node_entries, &entry);

	// Serialize and push the ports of the node.
	for (int j = 0; j < node->ports.length; j++) {
		port_t* port_element = dynamic_array_at(&node->ports, j);
		prv_port_serialize_and_push(port_element, graph, port_entries,
			string_buffer, data_buffer);
	}
}

daggle_error_code_t
daggle_graph_serialize(daggle_graph_h handle, unsigned char** out_bin,
	uint64_t* out_len)
{
	graph_t* graph = handle;

	dynamic_array_t node_entries;
	dynamic_array_t port_entries;
	dynamic_array_t string_buffer;
	dynamic_array_t data_buffer;

	const uint64_t num_nodes = graph->nodes.length;

	dynamic_array_init(num_nodes, sizeof(node_entry_1_t), &node_entries);
	dynamic_array_init(0, sizeof(node_entry_1_t), &port_entries);
	dynamic_array_init(0, sizeof(char), &string_buffer);
	dynamic_array_init(0, sizeof(unsigned char), &data_buffer);

	for (int i = 0; i < num_nodes; i++) {
		node_t** node_element = dynamic_array_at(&graph->nodes, i);
		prv_node_serialize_and_push(*node_element, graph, &node_entries,
			&port_entries, &string_buffer, &data_buffer);
	}

	prv_write_arrays_to_bin(&node_entries, &port_entries, &string_buffer,
		&data_buffer, out_bin, out_len);

	dynamic_array_destroy(&node_entries);
	dynamic_array_destroy(&port_entries);
	dynamic_array_destroy(&string_buffer);
	dynamic_array_destroy(&data_buffer);

	RETURN_STATUS(DAGGLE_SUCCESS);
}

typedef struct {
	uint64_t node_index;
	uint64_t port_index;
} prv_u64_tuple_t;

port_t*
prv_get_port_with_global_index(graph_t* graph, prv_u64_tuple_t* port_index_map,
	uint64_t index)
{
	prv_u64_tuple_t* indices = port_index_map + index;

	node_t* node
		= *((node_t**)dynamic_array_at(&graph->nodes, indices->node_index));

	port_t* port = dynamic_array_at(&node->ports, indices->port_index);

	return port;
}

void
prv_deserialize_port(daggle_instance_h instance, node_t* node,
	uint64_t global_index, uint64_t local_index, const port_entry_1_t* ports,
	char* strings, unsigned char* datas)
{
	port_t* port_element = dynamic_array_at(&node->ports, local_index);

	const port_entry_1_t* port_entry = ports + global_index;
	const char* port_name = strings + port_entry->name_stoff;

	printf("- Port: %s\n", port_name);
	printf("  - Index: port[%llu]\n", global_index);

	const char* pvarnames[] = { "INPUT", "OUTPUT", "PARAMETER" };
	printf("  - Variant: %s\n", pvarnames[port_entry->port_variant]);

	daggle_port_variant_t variant;
	variant = prv_port_variant_1_to_daggle(port_entry->port_variant);

	port_init(node, port_name, variant, port_element);

	// Deserialize input port variant
	if (port_entry->port_variant == INPUT) {
		const char* ivarnames[] = { "IMMUTABLE_REFERENCE", "IMMUTABLE_COPY",
			"MUTABLE_REFERENCE", "MUTABLE_COPY" };
		printf("  - Input: %s\n", ivarnames[port_entry->port_specific.input]);

		if (port_entry->edge_ptidx != UINT64_MAX) {
			printf("  - Link: port[%llu]\n", port_entry->edge_ptidx);
		}

		daggle_input_behavior_t input_behavior;
		input_behavior
			= prv_input_behavior_1_to_daggle(port_entry->port_specific.input);

		port_element->variant.input.behavior = input_behavior;
	}

	// If port has data deserialize it and set the port value
	if (port_entry->data_dtoff != UINT64_MAX) {
		data_entry_1_t* data_entry = (void*)datas + port_entry->data_dtoff;

		const char* data_type = strings + data_entry->type_stoff;

		printf("  - Data: %s (%lluB)\n", data_type, data_entry->size);

		void* deserialized_data = NULL;
		daggle_data_deserialize(instance, data_type, data_entry->bytes,
			data_entry->size, &deserialized_data);

		type_info_t* typeinfo;
		resource_container_get_type(
			&((instance_t*)instance)->plugin_manager.res, data_type, &typeinfo);

		data_container_replace(&port_element->value, typeinfo,
			deserialized_data);
	}
}

daggle_error_code_t
prv_graph_deserialize_1(daggle_instance_h instance, const unsigned char* bin,
	daggle_graph_h* out_graph)
{
	const graph_1_t* graph_bin = (void*)bin;

	const uint64_t version = graph_bin->version;
	const uint64_t num_nodes = graph_bin->num_nodes;
	const uint64_t num_ports = graph_bin->num_ports;
	const uint64_t text_len = graph_bin->text_len;
	const uint64_t data_len = graph_bin->data_len;

	const node_entry_1_t* nodes = (void*)&graph_bin->bytes;
	const port_entry_1_t* ports
		= (void*)nodes + sizeof(node_entry_1_t) * num_nodes;
	char* strings = (void*)ports + sizeof(port_entry_1_t) * num_ports;
	unsigned char* datas = (void*)strings + sizeof(char) * text_len;

	// Create port index map to convert global port index to the node index
	// and local port index.
	prv_u64_tuple_t* port_index_map
		= malloc(sizeof(prv_u64_tuple_t) * num_ports);

	graph_t* graph;
	daggle_graph_create(instance, (daggle_graph_h)&graph);

	printf("Version: %llu\nNodes: %llu\nPorts: %llu\n", version, num_nodes,
		num_ports);

	for (int node_index = 0; node_index < num_nodes; node_index++) {
		const node_entry_1_t* node_entry = nodes + node_index;

		const char* node_name = strings + node_entry->name_stoff;
		const char* node_type = strings + node_entry->type_stoff;

		printf("Node: %s (%s)\n", node_name, node_type);

		// Construct node
		node_info_t* info;
		resource_container_get_node(&graph->instance->plugin_manager.res,
			node_type, &info);

		node_t* node = malloc(sizeof *node);

		node->instance_task = NULL;
		node->info = info;
		node->graph = graph;
		node->custom_context = NULL;
		node->custom_context_destructor = NULL;

		// Initialize port array
		dynamic_array_init(node_entry->num_ports, sizeof(port_t), &node->ports);
		node->ports.length = node_entry->num_ports;

		for (int local_index = 0; local_index < node_entry->num_ports;
			local_index++) {
			uint64_t global_index = node_entry->first_port_ptidx + local_index;

			// Store the node and local port indices of this port.
			port_index_map[global_index].node_index = node_index;
			port_index_map[global_index].port_index = local_index;

			prv_deserialize_port(instance, node, global_index, local_index,
				ports, strings, datas);
		}

		dynamic_array_push(&graph->nodes, &node);
	}

	for (int i = 0; i < num_ports; i++) {
		const port_entry_1_t* port_entry = ports + i;

		// Continue to next port if this one is not linked.
		if (port_entry->edge_ptidx == UINT64_MAX) {
			continue;
		}

		// Get target port.
		port_t* target_port
			= prv_get_port_with_global_index(graph, port_index_map, i);

		// Ensure the target port is input.
		if (target_port->port_variant != DAGGLE_PORT_INPUT) {
			RETURN_STATUS(DAGGLE_ERROR_PARSE);
		}

		// Get the source port.
		port_t* source_port = prv_get_port_with_global_index(graph,
			port_index_map, port_entry->edge_ptidx);

		// Ensure the source port is output.
		if (source_port->port_variant != DAGGLE_PORT_OUTPUT) {
			RETURN_STATUS(DAGGLE_ERROR_PARSE);
		}

		daggle_port_connect(source_port, target_port);
	}

	free(port_index_map);

	for (int i = 0; i < num_nodes; i++) {
		node_t** node_element = dynamic_array_at(&graph->nodes, i);
		node_t* node = *node_element;

		node_compute_declarations(node);
	}

	*out_graph = graph;

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_graph_deserialize(daggle_instance_h instance, const unsigned char* bin,
	daggle_graph_h* out_graph)
{
	uint64_t* version = (void*)bin;

	if (*version == 1) {
		RETURN_STATUS(prv_graph_deserialize_1(instance, bin, out_graph));
	}

	LOG_FMT(LOG_TAG_ERROR, "Unsupported graph version %llu", *version);
	RETURN_STATUS(DAGGLE_ERROR_UNKNOWN);
}
