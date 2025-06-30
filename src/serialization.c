#include "serialization.h"

#include "graph.h"
#include "memory.h"
#include "node.h"
#include "ports.h"
#include "stdio.h"
#include "stdlib.h"
#include "utility/return_macro.h"

// Find the flattened index of a port.
uint64_t
prv_get_port_flat_index(
	const graph_t* graph, const port_t* port)
{
	uint64_t counter = 0;

	for(int i = 0; i < graph->nodes.length; i++) {
		node_t** node_element = dynamic_array_at(&graph->nodes, i);
		node_t* node = *node_element;

		for(int j = 0; j < node->ports.length; j++) {
			port_t* port_element = dynamic_array_at(&node->ports, j);

			if(port_element == port) {
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
prv_get_flat_port_indices(
	uint64_t edge_ptidx,
	uint64_t num_nodes,
	const node_entry_1_t* nodes,
	uint64_t* out_node_idx,
	uint64_t* out_port_idx)
{
	uint64_t counter = 0;
	for(int i = 0; i < num_nodes; i++) {
		const node_entry_1_t* node_entry = nodes + i;
		for(int j = 0; j < node_entry->num_ports; j++) {
			if(counter == edge_ptidx) {
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
prv_offset_write(
	void* target, const void* source, uint64_t stride, uint64_t* ref_offset)
{
	memcpy(target + *ref_offset, source, stride);
	*ref_offset += stride;
}

void
prv_write_arrays_to_bin(
	const dynamic_array_t* node_entries,
	const dynamic_array_t* port_entries,
	const dynamic_array_t* string_buffer,
	const dynamic_array_t* data_buffer,
	unsigned char** out_bin,
	uint64_t* out_len)
{
	uint64_t total_size = 0;

	total_size += sizeof(uint64_t) * 3;
	total_size += node_entries->stride * node_entries->length;
	total_size += node_entries->stride * port_entries->length;
	total_size += sizeof(uint64_t);
	total_size += string_buffer->stride * string_buffer->length;
	total_size += sizeof(uint64_t);
	total_size += data_buffer->stride * data_buffer->length;

	unsigned char* bin = malloc(total_size);
	uint64_t offset = 0;

	uint64_t version = 1;
	prv_offset_write(bin, &version, sizeof(uint64_t), &offset);

	prv_offset_write(bin, &node_entries->length, sizeof(uint64_t), &offset);
	prv_offset_write(bin, &port_entries->length, sizeof(uint64_t), &offset);

	prv_offset_write(bin,
		node_entries->data,
		node_entries->stride * node_entries->length,
		&offset);
	prv_offset_write(bin,
		port_entries->data,
		port_entries->stride * port_entries->length,
		&offset);

	prv_offset_write(bin, &string_buffer->length, sizeof(uint64_t), &offset);
	prv_offset_write(bin,
		string_buffer->data,
		string_buffer->stride * string_buffer->length,
		&offset);

	prv_offset_write(bin, &data_buffer->length, sizeof(uint64_t), &offset);
	prv_offset_write(bin,
		data_buffer->data,
		data_buffer->stride * data_buffer->length,
		&offset);

	*out_bin = bin;
	*out_len = total_size;
}

void
prv_append_string_buffer(
	dynamic_array_t* string_buffer, const char* source, uint64_t* out_index)
{
	char str_term = '\0';
	*out_index = string_buffer->length;

	for(const char* c = source; *c != '\0'; ++c) {
		char character = *c;
		dynamic_array_push(string_buffer, &character);
	}

	dynamic_array_push(string_buffer, &str_term);
}

daggle_error_code_t
daggle_graph_serialize(
	daggle_graph_h handle, unsigned char** out_bin, uint64_t* out_len)
{
	graph_t* graph = handle;

	dynamic_array_t node_entries;
	dynamic_array_init(
		graph->nodes.length, sizeof(node_entry_1_t), &node_entries);

	dynamic_array_t port_entries;
	dynamic_array_init(0, sizeof(node_entry_1_t), &port_entries);

	dynamic_array_t string_buffer;
	dynamic_array_init(0, sizeof(char), &string_buffer);

	dynamic_array_t data_buffer;
	dynamic_array_init(0, sizeof(unsigned char), &data_buffer);

	for(int i = 0; i < graph->nodes.length; i++) {
		node_t** node_element = dynamic_array_at(&graph->nodes, i);
		node_t* node = *node_element;

		node_entry_1_t entry;

		char str_term = '\0';

		// Push dummy string ID
		entry.name_stoff = string_buffer.length;
		char dummy = 'n';
		dynamic_array_push(&string_buffer, &dummy);
		dynamic_array_push(&string_buffer, &str_term);

		prv_append_string_buffer(
			&string_buffer, node->info->name_hash.name, &entry.type_stoff);

		entry.num_ports = node->ports.length;
		entry.first_port_ptidx = port_entries.length;

		for(int j = 0; j < node->ports.length; j++) {
			port_t* port = dynamic_array_at(&node->ports, j);

			port_entry_1_t port_entry;

			port_entry.name_stoff = 0;
			port_entry.edge_ptidx = 0;
			port_entry.data_dtoff = 0;
			port_entry.flags = 0;

			prv_append_string_buffer(
				&string_buffer, port->name_hash.name, &port_entry.name_stoff);

			switch(port->port_variant) {
			case DAGGLE_PORT_INPUT:
				port_entry.flags |= (1 << 0); // input
				port_entry.flags
					|= (0 << 3); // TODO: change to support input port variants 
				port_entry.flags
					|= ((port->variant.input.link != NULL) << 5); // linked

				if(port->variant.input.link) {
					port_entry.edge_ptidx = prv_get_port_flat_index(
						graph, port->variant.input.link);
				}
				break;
			case DAGGLE_PORT_OUTPUT:
				port_entry.flags |= (1 << 1); // output
				break;
			case DAGGLE_PORT_PARAMETER:
				port_entry.flags |= (1 << 2); // param
				break;
			default:
				break;
			}

			if(data_container_has_value(&port->value)) {
				port_entry.flags |= (1 << 4); // has_data

				unsigned char* data_bin;
				uint64_t data_len;

				daggle_data_serialize(graph->instance,
					port->value.info->name_hash.name,
					port->value.data,
					&data_bin,
					&data_len);

				// TODO: Instead of copying the type string, a common pool of
				// type strings could be used.
				uint64_t type_stoff = string_buffer.length;
				prv_append_string_buffer(&string_buffer,
					port->value.info->name_hash.name,
					&type_stoff);

				uint64_t data_entry_size
					= sizeof(uint64_t) + sizeof(uint64_t) + data_len;

				// Reserve additional space for the data entry
				dynamic_array_resize(
					&data_buffer, data_buffer.length + data_entry_size);

				// Copy data to the data buffer
				memcpy(data_buffer.data + data_buffer.length,
					&type_stoff,
					sizeof(uint64_t));
				memcpy(data_buffer.data + data_buffer.length + sizeof(uint64_t),
					&data_len,
					sizeof(uint64_t));
				memcpy(data_buffer.data + data_buffer.length + sizeof(uint64_t)
						   + sizeof(uint64_t),
					data_bin,
					data_len);

				port_entry.data_dtoff = data_buffer.length;

				data_buffer.length += data_entry_size;
			}

			dynamic_array_push(&port_entries, &port_entry);
		}

		dynamic_array_push(&node_entries, &entry);
	}

	prv_write_arrays_to_bin(&node_entries,
		&port_entries,
		&string_buffer,
		&data_buffer,
		out_bin,
		out_len);

	dynamic_array_destroy(&node_entries);
	dynamic_array_destroy(&port_entries);
	dynamic_array_destroy(&string_buffer);
	dynamic_array_destroy(&data_buffer);

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
prv_graph_deserialize_1(
	daggle_instance_h instance,
	const unsigned char* bin,
	daggle_graph_h* out_graph)
{
	const uint64_t* version = (void*)bin;

	ASSERT_TRUE(*version == 1, "Wrong graph version");

	const uint64_t* num_nodes = (void*)version + sizeof(uint64_t);
	const uint64_t* num_ports = (void*)num_nodes + sizeof(uint64_t);

	const node_entry_1_t* nodes = (void*)num_ports + sizeof(uint64_t);
	const port_entry_1_t* ports
		= (void*)nodes + sizeof(node_entry_1_t) * *num_nodes;

	const uint64_t* strings_len
		= (void*)ports + sizeof(port_entry_1_t) * *num_ports;
	char* strings = (void*)strings_len + sizeof(uint64_t);

	const uint64_t* datas_len = (void*)strings + *strings_len;
	unsigned char* datas = (void*)datas_len + sizeof(uint64_t);

	graph_t* graph;
	daggle_graph_create(instance, (daggle_graph_h)&graph);

	// printf("Version: %zu\nNodes: %zu\nPorts: %zu\n", *version, *num_nodes,
	// *num_ports);

	for(int i = 0; i < *num_nodes; i++) {
		const node_entry_1_t* node_entry = nodes + i;

		const char* node_name = strings + node_entry->name_stoff;
		const char* node_type = strings + node_entry->type_stoff;

		// printf("Node: %s (%s)\n", node_name, node_type);

		// Construct node
		node_info_t* info;
		resource_container_get_node(
			&graph->instance->plugin_manager.res, node_type, &info);

		node_t* node = malloc(sizeof *node);

		node->instance_task = NULL;
		node->info = info;
		node->graph = graph;
		node->custom_context = NULL;
		node->custom_context_destructor = NULL;

		// Initialize port array
		dynamic_array_init(node_entry->num_ports, sizeof(port_t), &node->ports);
		node->ports.length = node_entry->num_ports;

		for(int j = 0; j < node_entry->num_ports; j++) {
			port_t* port_element = dynamic_array_at(&node->ports, j);
			const port_entry_1_t* port_entry
				= ports + node_entry->first_port_ptidx + j;

			const char* port_name = strings + port_entry->name_stoff;

			// printf("- Port: %s\n", port_name);

			daggle_port_variant_t variant;

			if(port_entry->flags & (1 << 0)) {
				variant = DAGGLE_PORT_INPUT;
			} else if(port_entry->flags & (1 << 1)) {
				variant = DAGGLE_PORT_OUTPUT;
			} else if(port_entry->flags & (1 << 2)) {
				variant = DAGGLE_PORT_PARAMETER;
			}

			data_container_t data;
			data_container_init(instance, &data);

			// If port has data
			if(port_entry->flags & (1 << 4)) {
				uint64_t* data_type_stoff
					= (void*)datas + port_entry->data_dtoff;
				uint64_t* data_size = (void*)data_type_stoff + sizeof(uint64_t);
				unsigned char* data_bytes = (void*)data_size + sizeof(uint64_t);

				const char* data_type = strings + *data_type_stoff;

				// printf("  - Data: %s (%zuB)\n", data_type, *data_size);

				void* deserialized_data = NULL;
				daggle_data_deserialize(instance,
					data_type,
					data_bytes,
					*data_size,
					&deserialized_data);

				type_info_t* typeinfo;
				resource_container_get_type(
					&graph->instance->plugin_manager.res, data_type, &typeinfo);

				data_container_replace(&data, typeinfo, deserialized_data);
			}

			port_init(node, port_name, data, variant, port_element);

			// TODO: deserialize the input port variant
			if(port_entry->flags & (1 << 0)) {
				// Not implemented yet, fallback to immutable references
				port_element->variant.input.variant = DAGGLE_INPUT_IMMUTABLE_REFERENCE;
			}
		}

		dynamic_array_push(&graph->nodes, &node);
	}

	for(int i = 0; i < *num_nodes; i++) {
		node_t** node_element = dynamic_array_at(&graph->nodes, i);
		node_t* node = *node_element;
		const node_entry_1_t* node_entry = nodes + i;

		for(int j = 0; j < node_entry->num_ports; j++) {
			port_t* port_element = dynamic_array_at(&node->ports, j);
			const port_entry_1_t* port_entry
				= ports + node_entry->first_port_ptidx + j;

			if(port_entry->flags & (1 << 5)) {
				uint64_t node_idx = 0;
				uint64_t port_idx = 0;

				if(*num_ports < port_entry->edge_ptidx) {
					LOG(LOG_TAG_ERROR, "Connected port not found");
				}

				prv_get_flat_port_indices(port_entry->edge_ptidx,
					*num_nodes,
					nodes,
					&node_idx,
					&port_idx);

				node_t* source_node
					= *((node_t**)dynamic_array_at(&graph->nodes, node_idx));
				port_t* source_port
					= dynamic_array_at(&source_node->ports, port_idx);

				if(source_port->port_variant != DAGGLE_PORT_OUTPUT) {
					LOG(LOG_TAG_ERROR, "Invalid connection");
				}

				daggle_port_connect(port_element, source_port);
			}
		}
	}

	for(int i = 0; i < *num_nodes; i++) {
		node_t** node_element = dynamic_array_at(&graph->nodes, i);
		node_t* node = *node_element;

		node_compute_declarations(node);
	}

	*out_graph = graph;

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_graph_deserialize(
	daggle_instance_h instance,
	const unsigned char* bin,
	daggle_graph_h* out_graph)
{
	uint64_t* version = (void*)bin;

	if(*version == 1) {
		RETURN_STATUS(prv_graph_deserialize_1(instance, bin, out_graph));
	}

	LOG_FMT(LOG_TAG_ERROR, "Unsupported graph version %zu", version);
	RETURN_STATUS(DAGGLE_ERROR_UNKNOWN);
}
