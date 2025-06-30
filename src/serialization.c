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
    switch (variant)
    {
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
    switch (variant)
    {
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

input_variant_1_t
prv_input_variant_daggle_to_1(daggle_input_variant_t variant)
{
    switch (variant)
    {
    default:
        LOG_FMT(LOG_TAG_ERROR, "Unknown input variant %u", variant);
    case DAGGLE_INPUT_IMMUTABLE_REFERENCE:
        return IMMUTABLE_REFERENCE;
    case DAGGLE_INPUT_IMMUTABLE_COPY:
        return IMMUTABLE_COPY;
    case DAGGLE_INPUT_MUTABLE_REFERENCE:
        return MUTABLE_REFERENCE;
    case DAGGLE_INPUT_MUTABLE_COPY:
        return MUTABLE_COPY;
    }
}

daggle_input_variant_t
prv_input_variant_1_to_daggle(input_variant_1_t variant)
{
    switch (variant)
    {
    default:
        LOG_FMT(LOG_TAG_ERROR, "Unknown input variant %u", variant);
    case IMMUTABLE_REFERENCE:
        return DAGGLE_INPUT_IMMUTABLE_REFERENCE;
    case IMMUTABLE_COPY:
        return DAGGLE_INPUT_IMMUTABLE_COPY;
    case MUTABLE_REFERENCE:
        return DAGGLE_INPUT_MUTABLE_REFERENCE;
    case MUTABLE_COPY:
        return DAGGLE_INPUT_MUTABLE_COPY;
    }
}

// Find the flattened index of a port.
uint64_t
prv_get_port_flat_index(const graph_t* graph, const port_t* port)
{
    uint64_t counter = 0;

    for (int i = 0; i < graph->nodes.length; i++)
    {
        node_t** node_element = dynamic_array_at(&graph->nodes, i);
        node_t* node = *node_element;

        for (int j = 0; j < node->ports.length; j++)
        {
            port_t* port_element = dynamic_array_at(&node->ports, j);

            if (port_element == port)
            {
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
void prv_get_flat_port_indices(
    uint64_t edge_ptidx,
    uint64_t num_nodes,
    const node_entry_1_t* nodes,
    uint64_t* out_node_idx,
    uint64_t* out_port_idx)
{
    uint64_t counter = 0;
    for (int i = 0; i < num_nodes; i++)
    {
        const node_entry_1_t* node_entry = nodes + i;
        for (int j = 0; j < node_entry->num_ports; j++)
        {
            if (counter == edge_ptidx)
            {
                *out_port_idx = j;
                *out_node_idx = i;
                return;
            }

            counter++;
        }
    }

    printf("Port counting failed");
}

void prv_offset_write(
    void* target, const void* source, uint64_t stride, uint64_t* ref_offset)
{
    memcpy(target + *ref_offset, source, stride);
    *ref_offset += stride;
}

void prv_write_arrays_to_bin(
    const dynamic_array_t* node_entries,
    const dynamic_array_t* port_entries,
    const dynamic_array_t* string_buffer,
    const dynamic_array_t* data_buffer,
    unsigned char** out_bin,
    uint64_t* out_len)
{
    uint64_t total_size = 0;

    total_size += sizeof(uint64_t);
    total_size += sizeof(uint64_t);
    total_size += sizeof(uint64_t);
    total_size += sizeof(uint64_t);
    total_size += sizeof(uint64_t);
    total_size += node_entries->stride * node_entries->length;
    total_size += node_entries->stride * port_entries->length;
    total_size += string_buffer->stride * string_buffer->length;
    total_size += data_buffer->stride * data_buffer->length;

    unsigned char* bin = malloc(total_size);
    uint64_t offset = 0;

    uint64_t version = 1;
    prv_offset_write(bin, &version, sizeof(uint64_t), &offset);

    prv_offset_write(bin, &node_entries->length, sizeof(uint64_t), &offset);
    prv_offset_write(bin, &port_entries->length, sizeof(uint64_t), &offset);
    prv_offset_write(bin, &string_buffer->length, sizeof(uint64_t), &offset);
    prv_offset_write(bin, &data_buffer->length, sizeof(uint64_t), &offset);

    prv_offset_write(bin, node_entries->data, 
        node_entries->stride * node_entries->length, &offset);
        
    prv_offset_write(bin, port_entries->data,
        port_entries->stride * port_entries->length, &offset);

    prv_offset_write(bin, string_buffer->data,
        string_buffer->stride * string_buffer->length, &offset);

    prv_offset_write(bin, data_buffer->data, 
        data_buffer->stride * data_buffer->length,  &offset);

    *out_bin = bin;
    *out_len = total_size;
}

void prv_append_string_buffer(
    dynamic_array_t* string_buffer, const char* source, uint64_t* out_index)
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

void prv_append_data_buffer(
    dynamic_array_t* data_buffer, dynamic_array_t* string_buffer, const char* data_type, unsigned char* data_bin,
    uint64_t data_len, uint64_t* out_dtoff)
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

daggle_error_code_t
daggle_graph_serialize(
    daggle_graph_h handle, unsigned char** out_bin, uint64_t* out_len)
{
    graph_t* graph = handle;

    dynamic_array_t node_entries;
    dynamic_array_init(graph->nodes.length, sizeof(node_entry_1_t), 
        &node_entries);

    dynamic_array_t port_entries;
    dynamic_array_init(0, sizeof(node_entry_1_t), &port_entries);

    dynamic_array_t string_buffer;
    dynamic_array_init(0, sizeof(char), &string_buffer);

    dynamic_array_t data_buffer;
    dynamic_array_init(0, sizeof(unsigned char), &data_buffer);

    for (int i = 0; i < graph->nodes.length; i++)
    {
        node_t** node_element = dynamic_array_at(&graph->nodes, i);
        node_t* node = *node_element;

        node_entry_1_t entry;

        // Push dummy node id
        const char dummy[] = "dummy_id";
        prv_append_string_buffer(&string_buffer, dummy, &entry.name_stoff);

        // Push node type
        prv_append_string_buffer(
            &string_buffer, node->info->name_hash.name, &entry.type_stoff);

        entry.num_ports = node->ports.length;
        entry.first_port_ptidx = port_entries.length;

        dynamic_array_push(&node_entries, &entry);

        for (int j = 0; j < node->ports.length; j++)
        {
            port_t* port = dynamic_array_at(&node->ports, j);

            port_entry_1_t port_entry;

            // Initialize port entry
            port_entry.name_stoff = 0;
            port_entry.edge_ptidx = UINT64_MAX; // Unset is MAX
            port_entry.data_dtoff = UINT64_MAX;

            // Write port name to string buffer
            // and store character offset to name_stoff.
            prv_append_string_buffer(
                &string_buffer, port->name_hash.name, &port_entry.name_stoff);

            port_entry.port_variant = prv_port_variant_daggle_to_1(port->port_variant);

            // For input ports, set the input variant, and link if it has one
            if (port->port_variant == DAGGLE_PORT_INPUT)
            {
                port_entry.port_specific.input = prv_input_variant_daggle_to_1(
                    port->variant.input.variant);

                if (port->variant.input.link)
                {
                    port_entry.edge_ptidx = prv_get_port_flat_index(graph, 
                        port->variant.input.link);
                }
            }

            if (data_container_has_value(&port->value))
            {
                unsigned char* data_bin;
                uint64_t data_len;

                const char* data_type = port->value.info->name_hash.name;

                daggle_data_serialize(graph->instance, data_type,
                                      port->value.data, &data_bin, &data_len);

                prv_append_data_buffer(&data_buffer, &string_buffer, data_type, 
                    data_bin, data_len, &port_entry.data_dtoff);
            }

            dynamic_array_push(&port_entries, &port_entry);
        }
    }

    prv_write_arrays_to_bin(&node_entries, &port_entries, &string_buffer,
        &data_buffer, out_bin, out_len);

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

    uint64_t* nums = (void*)version + sizeof(uint64_t);

    const uint64_t num_nodes = nums[0];
    const uint64_t num_ports = nums[1];
    const uint64_t strings_len = nums[2];
    const uint64_t datas_len = nums[3];

    const node_entry_1_t* nodes = (void*)nums + 4 * sizeof(uint64_t); 
    const port_entry_1_t* ports = (void*)nodes + sizeof(node_entry_1_t) * num_nodes;
    char* strings = (void*)ports + sizeof(port_entry_1_t) * num_ports;
    unsigned char* datas = (void*)strings + sizeof(char) * strings_len;

    graph_t* graph;
    daggle_graph_create(instance, (daggle_graph_h)&graph);

    printf("Version: %llu\nNodes: %llu\nPorts: %llu\n", *version, num_nodes, num_ports);

    for (int i = 0; i < num_nodes; i++)
    {
        const node_entry_1_t* node_entry = nodes + i;

        const char* node_name = strings + node_entry->name_stoff;
        const char* node_type = strings + node_entry->type_stoff;

        printf("Node: %s (%s)\n", node_name, node_type);

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

        for (int j = 0; j < node_entry->num_ports; j++)
        {
            port_t* port_element = dynamic_array_at(&node->ports, j);
            const port_entry_1_t* port_entry = ports + node_entry->first_port_ptidx + j;

            const char* port_name = strings + port_entry->name_stoff;

            printf("- Port: %s\n", port_name);
            printf("  - Index: port[%llu]\n", node_entry->first_port_ptidx + j);

            const char* pvarnames[] = {"INPUT", "OUTPUT", "PARAMETER"};
            printf("  - Variant: %s\n", pvarnames[port_entry->port_variant]);

            daggle_port_variant_t variant;
            variant = prv_port_variant_1_to_daggle(port_entry->port_variant);

            data_container_t data;
            data_container_init(instance, &data);

            // If port has data
            if (port_entry->data_dtoff != UINT64_MAX)
            {
                data_entry_1_t* data_entry = (void*)datas + port_entry->data_dtoff;

                const char* data_type = strings + data_entry->type_stoff;

                printf("  - Data: %s (%lluB)\n", data_type, data_entry->size);

                void* deserialized_data = NULL;
                daggle_data_deserialize(instance,
                                        data_type,
                                        data_entry->bytes,
                                        data_entry->size,
                                        &deserialized_data);

                type_info_t* typeinfo;
                resource_container_get_type(
                    &graph->instance->plugin_manager.res, data_type, &typeinfo);

                data_container_replace(&data, typeinfo, deserialized_data);
            }

            port_init(node, port_name, data, variant, port_element);

            // Deserialize input port variant
            if (port_entry->port_variant == INPUT)
            {
                const char* ivarnames[] = {"IMMUTABLE_REFERENCE", "IMMUTABLE_COPY", "MUTABLE_REFERENCE", "MUTABLE_COPY"};
                printf("  - Input: %s\n", ivarnames[port_entry->port_specific.input]);

                if (port_entry->edge_ptidx != UINT64_MAX)
                {
                    printf("  - Link: port[%llu]\n", port_entry->edge_ptidx);
                }

                daggle_input_variant_t input_variant;
                input_variant = prv_input_variant_1_to_daggle(port_entry->port_specific.input);
                port_element->variant.input.variant = input_variant;
            }
        }

        dynamic_array_push(&graph->nodes, &node);
    }

    for (int i = 0; i < num_nodes; i++)
    {
        node_t** node_element = dynamic_array_at(&graph->nodes, i);
        node_t* node = *node_element;
        const node_entry_1_t* node_entry = nodes + i;

        for (int j = 0; j < node_entry->num_ports; j++)
        {
            port_t* port_element = dynamic_array_at(&node->ports, j);
            const port_entry_1_t* port_entry = ports + node_entry->first_port_ptidx + j;

            if (port_entry->edge_ptidx != UINT64_MAX)
            {
                uint64_t node_idx = 0;
                uint64_t port_idx = 0;

                if (num_ports < port_entry->edge_ptidx)
                {
                    LOG(LOG_TAG_ERROR, "Connected port not found");
                }

                prv_get_flat_port_indices(port_entry->edge_ptidx,
                                          num_nodes,
                                          nodes,
                                          &node_idx,
                                          &port_idx);

                node_t* source_node = *((node_t**)dynamic_array_at(&graph->nodes, node_idx));
                port_t* source_port = dynamic_array_at(&source_node->ports, port_idx);

                if (source_port->port_variant != DAGGLE_PORT_OUTPUT)
                {
                    LOG(LOG_TAG_ERROR, "Invalid connection");
                }

                daggle_port_connect(port_element, source_port);
            }
        }
    }

    for (int i = 0; i < num_nodes; i++)
    {
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

    if (*version == 1)
    {
        RETURN_STATUS(prv_graph_deserialize_1(instance, bin, out_graph));
    }

    LOG_FMT(LOG_TAG_ERROR, "Unsupported graph version %llu", *version);
    RETURN_STATUS(DAGGLE_ERROR_UNKNOWN);
}
