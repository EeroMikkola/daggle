#include "executor.h"
#include "task.h"
#include "graph.h"
#include "instance.h"
#include "node.h"
#include "ports.h"
#include "stdatomic.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "utility/dynamic_array.h"
#include "utility/log_macro.h"
#include "utility/return_macro.h"
#include "utility/thread_safe_linked_queue.h"

#include <daggle/daggle.h>
#include <stdio.h>
#include "string.h"

void
prv_graph_master_task_start(daggle_task_h task, void* context)
{
	ASSERT_NOT_NULL(context, "context is null");
}

void
prv_graph_master_task_complete(daggle_task_h task, void* context)
{
	ASSERT_NOT_NULL(context, "context is null");

	graph_t* graph = context;
	graph->locked = false;
}

void
prv_node_on_start(daggle_task_h task, void* context) {
	node_t* node = context;
	for (uint64_t i = 0; i < node->ports.length; ++i) {
		port_t* port = dynamic_array_at(&node->ports, i);
		if(port->port_variant == DAGGLE_PORT_INPUT) {
			port->variant.input.has_spent_access = false;
		} else if(port->port_variant == DAGGLE_PORT_OUTPUT) {
			atomic_store(&port->variant.output.num_pending_accesses, 
				port->variant.output.links.length);
		}
	}
}

void
prv_node_on_complete(daggle_task_h task, void* context) {
	node_t* node = context;
	for (uint64_t i = 0; i < node->ports.length; ++i) {
		port_t* port = dynamic_array_at(&node->ports, i);
		if(port->port_variant == DAGGLE_PORT_INPUT 
				&& port->variant.input.behavior == DAGGLE_INPUT_BEHAVIOR_REFERENCE 
				&& port->variant.input.link) {
			port_t* link = port->variant.input.link;
			atomic_fetch_sub(&link->variant.output.num_pending_accesses, 1);
		}
	}
}

daggle_error_code_t
prv_nodes_taskify(graph_t* graph, daggle_task_h* out_task)
{
	ASSERT_PARAMETER(graph);
	ASSERT_OUTPUT_PARAMETER(out_task);

	dynamic_array_t* nodes = &graph->nodes;

	if (graph->locked) {
		RETURN_STATUS(DAGGLE_ERROR_OBJECT_LOCKED);
	}

	if (nodes->length == 0) {
		LOG(LOG_TAG_ERROR,
			"At least one node must be defined to execute a graph");
		RETURN_STATUS(DAGGLE_ERROR_UNKNOWN);
	}

	daggle_error_code_t error = DAGGLE_SUCCESS;
	dynamic_array_t tasks;
	error = dynamic_array_init(nodes->length, sizeof(task_t*), &tasks);
	GOTO_IF_ERROR(error, list_error);

	graph->locked = true;

	// Create the tasks.
	for (uint64_t i = 0; i < nodes->length; ++i) {
		node_t* node = *(node_t**)dynamic_array_at(nodes, i);
		task_t* task = node->task;

		// Create task dependencies based on node links.
		for (uint64_t j = 0; j < node->ports.length; ++j) {
			port_t* port = dynamic_array_at(&node->ports, j);

			// Dependencies are created with output ports only.
			if (port->port_variant != DAGGLE_PORT_OUTPUT) {
				continue;
			}

			// For each link in the port.
			dynamic_array_t* links = &port->variant.output.links;
			for (uint64_t k = 0; k < links->length; ++k) {
				port_t* link = *(port_t**)dynamic_array_at(links, k);

				// Get the task of the linked node.
				node_t* owner = link->owner;
				task_t* dependant_task = owner->task;

				error = daggle_task_depend(dependant_task, task);
				GOTO_IF_ERROR(error, node_error);
			}
		}

		// Create a wrapper context for the task
		// TODO: Add memory error check. 
		// TODO: Could be optimized with a custom wrapper function + allocate all contexts as array, managed by master task.
		task_add_callback_wrapper(task, prv_node_on_start, prv_node_on_complete, NULL, node);

		dynamic_array_push(&tasks, &task);
	}

	// Remove tasks from nodes
	for (uint64_t i = 0; i < nodes->length; ++i) {
		node_t* node = *(node_t**)dynamic_array_at(nodes, i);
		node->task = NULL;
	}

	// Create a master task for executing the entire graph
	daggle_task_h master_task;
	daggle_task_create(prv_graph_master_task_start, 
		prv_graph_master_task_complete, NULL, graph,
		 "master", &master_task);
	daggle_task_add_subgraph(master_task, tasks.data, tasks.length);

	// Free the task array.
	dynamic_array_destroy(&tasks);

	*out_task = master_task;

	RETURN_STATUS(DAGGLE_SUCCESS);

node_error:
	for (uint64_t i = 0; i <= tasks.length; ++i) {
		task_t* task = *(task_t**)dynamic_array_at(&tasks, i);
		task_free(task);
	}

	dynamic_array_destroy(&tasks);

list_error:

	RETURN_STATUS(DAGGLE_ERROR_UNKNOWN);
}

daggle_error_code_t
daggle_graph_taskify(daggle_graph_h graph, daggle_task_h* out_task)
{
	REQUIRE_PARAMETER(graph);
	REQUIRE_OUTPUT_PARAMETER(out_task);

	daggle_task_h task;
	RETURN_IF_ERROR(prv_nodes_taskify(graph, &task));

	*out_task = task;

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_graph_execute(daggle_instance_h instance, daggle_graph_h graph)
{
	REQUIRE_PARAMETER(instance);
	REQUIRE_PARAMETER(graph);

	daggle_task_h task;
	daggle_graph_taskify(graph, &task);
	daggle_task_execute(instance, task);
	while (((graph_t*)graph)->locked) {
	
	}

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_task_execute(daggle_instance_h instance, daggle_task_h task)
{
	REQUIRE_PARAMETER(instance);
	REQUIRE_PARAMETER(task);

	instance_t* instance_impl = instance;

	ts_llist_queue_enqueue(&instance_impl->executor.queue, task);

	task_t* t = task;

	// Note: this is a hack, and sometimes exists too quickly
	// TODO: come up with a better solution
	while (atomic_load(&t->execution.num_pending_subtasks) > 0) { }; 

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_graph_get_daggle(daggle_graph_h graph, daggle_instance_h* out_daggle)
{
	REQUIRE_PARAMETER(graph);
	REQUIRE_OUTPUT_PARAMETER(out_daggle);

	graph_t* graph_impl = graph;
	instance_t* instance = graph_impl->instance;

	*out_daggle = instance;

	RETURN_STATUS(DAGGLE_SUCCESS);
}
