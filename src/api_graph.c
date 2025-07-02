#include "data_container.h"
#include "graph.h"
#include "instance.h"
#include "node.h"
#include "stdatomic.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "utility/dynamic_array.h"
#include "utility/hash.h"
#include "utility/return_macro.h"

daggle_error_code_t
daggle_graph_create(daggle_instance_h instance, daggle_graph_h* out_graph)
{
	REQUIRE_PARAMETER(instance);
	REQUIRE_OUTPUT_PARAMETER(out_graph);

	graph_t* graph = malloc(sizeof(graph_t));
	REQUIRE_ALLOCATION_DAGGLE_SUCCESSFUL(graph);

	// Initialize node list with 0 capacity (success guaranteed)
	dynamic_array_init(0, sizeof(port_t), &graph->nodes);

	graph->instance = instance;
	graph->owner = NULL;
	graph->locked = false;

	*out_graph = graph;

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_graph_free(daggle_graph_h handle)
{
	REQUIRE_PARAMETER(handle);

	graph_t* graph = handle;
	dynamic_array_destroy(&graph->nodes);

	graph->instance = NULL;
	graph->owner = NULL;

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_graph_add_node(daggle_graph_h handle, const char* type,
	daggle_node_h* out_node)
{
	REQUIRE_PARAMETER(handle);
	REQUIRE_OUTPUT_PARAMETER(out_node);

	graph_t* graph = handle;

	if (graph->locked) {
		RETURN_STATUS(DAGGLE_ERROR_OBJECT_LOCKED);
	}

	node_t* node;
	RETURN_IF_ERROR(node_create(graph, type, &node));

	RETURN_IF_ERROR(
		dynamic_array_push(&graph->nodes, &node)); // TODO: Free node if error

	*out_node = node;

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_graph_remove_node(daggle_graph_h handle, daggle_node_h node)
{
	REQUIRE_PARAMETER(handle);
	REQUIRE_PARAMETER(node);

	graph_t* graph = handle;

	if (graph->locked) {
		RETURN_STATUS(DAGGLE_ERROR_OBJECT_LOCKED);
	}

	for (uint64_t i = 0; i < graph->nodes.length; ++i) {
		node_t** item = dynamic_array_at(&graph->nodes, i);

		if (item == node) {
			dynamic_array_remove(&graph->nodes, i);

			node_free(node);

			RETURN_STATUS(DAGGLE_SUCCESS);
		}
	}

	LOG(LOG_TAG_ERROR, "Node to remove not part of graph");
	RETURN_STATUS(DAGGLE_ERROR_UNKNOWN);
}

daggle_error_code_t
daggle_graph_get_node_by_index(daggle_graph_h handle, uint64_t index,
	daggle_node_h* out_node)
{
	REQUIRE_PARAMETER(handle);
	REQUIRE_OUTPUT_PARAMETER(out_node);

	graph_t* graph = handle;
	if (index >= graph->nodes.length) {
		*out_node = NULL;
		RETURN_STATUS(DAGGLE_SUCCESS);
	}

	node_t** node = dynamic_array_at(&graph->nodes, index);

	// If the port does not exist, NULL will be written.
	*out_node = *node;
	RETURN_STATUS(DAGGLE_SUCCESS);
}