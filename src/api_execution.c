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

	dynamic_array_t tasks;
	GOTO_IF_ERROR(dynamic_array_init(nodes->length, sizeof(task_t*), &tasks), list_error);

	graph->locked = true;

	// Create the tasks.
	for (uint64_t i = 0; i < nodes->length; ++i) {
		node_t* node = *(node_t**)dynamic_array_at(nodes, i);
		task_t* task = node->task;

		// Create task dependencies based on node links.
		for (uint64_t j = 0; j < node->ports.length; ++j) {
			port_t* port = dynamic_array_at(&node->ports, j);

			if (port->port_variant != DAGGLE_PORT_INPUT) {
				continue;
			}

			port_t* linked_port = port->variant.input.link;
			if(!linked_port) {
				continue;
			}

			node_t* linked_node = linked_port->owner;
			task_t* dependant_task = linked_node->task;

			GOTO_IF_ERROR(daggle_task_depend(task, dependant_task), node_error);
		}

		// Create a wrapper context for the task
		// TODO: Could be optimized with a custom wrapper function + allocate all contexts as array, managed by master task.
		GOTO_IF_ERROR(task_add_callback_wrapper(task, prv_node_on_start, prv_node_on_complete, NULL, node), node_error);

		// Add the task to the task array.
		// Note: should never error out, as the array has been preallocated.
		GOTO_IF_ERROR(dynamic_array_push(&tasks, &task), node_error);
	}

	// Remove tasks from nodes
	for (uint64_t i = 0; i < nodes->length; ++i) {
		node_t* node = *(node_t**)dynamic_array_at(nodes, i);
		node->task = NULL;
	}

	// Create a master task for executing the entire graph
	daggle_task_h master_task;
	GOTO_IF_ERROR(daggle_task_create(prv_graph_master_task_start, prv_graph_master_task_complete, NULL, graph, "master", &master_task), node_error);

	// Make graph the subtasks of the master task.
	GOTO_IF_ERROR(daggle_task_add_subgraph(master_task, tasks.data, tasks.length), master_error);

	// Free the task array.
	dynamic_array_destroy(&tasks);

	*out_task = master_task;

	RETURN_STATUS(DAGGLE_SUCCESS);

master_error:
	task_free(master_task);

node_error:
	for (uint64_t i = 0; i <= tasks.length; ++i) {
		task_t* task = *(task_t**)dynamic_array_at(&tasks, i);
		task_free(task);
	}

	dynamic_array_destroy(&tasks);

list_error: 
	*out_task = NULL;
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
