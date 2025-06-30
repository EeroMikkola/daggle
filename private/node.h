#pragma once

#include "ports.h"
#include "resource_container.h"

#include <daggle/daggle.h>

typedef struct node_s {
	node_info_t* info;
	dynamic_array_t ports;
	daggle_node_task_fn instance_task;

	daggle_graph_h graph;

	void* custom_context;
	daggle_node_context_free_fn custom_context_destructor;
} node_t;

daggle_error_code_t
node_create(daggle_graph_h graph, const char* type, node_t** out_node);

void
node_free(node_t* node);

port_t*
node_get_port_by_name(node_t* node, const char* port);

port_t*
node_get_port_by_index(node_t* node, uint64_t index);

void
node_compute_declarations(node_t* node);
