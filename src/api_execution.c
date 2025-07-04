#include "executor.h"
#include "graph.h"
#include "instance.h"
#include "node.h"
#include "ports.h"
#include "stdatomic.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "utility/closure.h"
#include "utility/dynamic_array.h"
#include "utility/return_macro.h"
#include "utility/thread_safe_linked_queue.h"

#include <daggle/daggle.h>

void
prv_graph_master_task_function(void* context)
{
	ASSERT_NOT_NULL(context, "context is null");
	LOG(LOG_TAG_INFO, "Run Graph");
}

void
prv_graph_master_task_dispose(void* context)
{
	ASSERT_NOT_NULL(context, "context is null");
	graph_t* graph = context;

	LOG(LOG_TAG_INFO, "Finish Graph");

	graph->locked = false;
}

void
prv_node_call_function(daggle_task_h task, void* context)
{
	node_t* node = context;
	node->instance_task(task, node->custom_context);

	// Subtract reference accesses.
	for (uint64_t i = 0; i < node->ports.length; ++i) {
		port_t* port = dynamic_array_at(&node->ports, i);
		if(port->port_variant == DAGGLE_PORT_INPUT && port->variant.input.behavior == DAGGLE_INPUT_BEHAVIOR_REFERENCE && port->variant.input.link) {
			port_t* link = port->variant.input.link;
			atomic_fetch_sub(&link->variant.output.num_pending_accesses, 1);
		}
	}
}

void
prv_node_call_dispose(void* context)
{
	node_t* node = context;
	if(node->custom_context_destructor) {
		node->custom_context_destructor(node->custom_context);
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
	GOTO_IF_ERROR(error, node_error);

	graph->locked = true;

	// Create the tasks.
	for (uint64_t i = 0; i < nodes->length; ++i) {
		node_t** nodeelem = dynamic_array_at(nodes, i);
		node_t* node = *nodeelem;

		// TODO: move to a more appropriate location.
		// Reset port counters.
		for (uint64_t j = 0; j < node->ports.length; ++j) {
			port_t* port = dynamic_array_at(&node->ports, j);

			if(port->port_variant == DAGGLE_PORT_INPUT) {
				port->variant.input.has_spent_access = false;
			} else if(port->port_variant == DAGGLE_PORT_OUTPUT) {
				atomic_store(&port->variant.output.num_pending_accesses, 
					port->variant.output.links.length);
			}
		}

		task_t* tk;
		daggle_task_create(prv_node_call_function, prv_node_call_dispose, node,
			(char*)node->info->name_hash.name, (daggle_task_h*)&tk);

		// TODO: handle error, must task_free(tk) every initialized array
		dynamic_array_push(&tasks, &tk);
	}

	// Construct dependencies with node links.
	for (uint64_t i = 0; i < tasks.length; ++i) {
		task_t** tkelem = dynamic_array_at(&tasks, i);
		task_t* tk = *tkelem;

		node_t** nodeelem = dynamic_array_at(nodes, i);
		node_t* node = *nodeelem;

		// For each output port of the node.
		for (uint64_t j = 0; j < node->ports.length; ++j) {
			port_t* item = dynamic_array_at(&node->ports, j);
			if (item->port_variant != DAGGLE_PORT_OUTPUT) {
				continue;
			}

			// For each link in the port.
			dynamic_array_t* links = &item->variant.output.links;
			for (uint64_t j = 0; j < links->length; ++j) {
				port_t** linkelem = dynamic_array_at(links, j);

				// Get the owner node of the port.
				node_t* owner = (*linkelem)->owner;

				// Find the tark corresponding to the linked node.
				task_t* task = NULL;
				for (uint64_t k = 0; k < nodes->length; ++k) {
					node_t** ndelem = dynamic_array_at(nodes, k);
					node_t* n = *ndelem;

					if (n == owner) {
						task_t** tklm = dynamic_array_at(&tasks, k);
						task = *tklm;
						break;
					}

					task = NULL;
				}

				// Skip if the task was not found.
				// i.e. A->B, where only A is executed.
				if (!task) {
					LOG(LOG_TAG_ERROR, "NOT FOUND");
					continue;
				}

				error = daggle_task_depend(task, tk);
				GOTO_IF_ERROR(error, node_error);
			}
		}
	}

	task_t* master_task = malloc(sizeof(task_t));
	master_task->tail = NULL;
	master_task->head = NULL;

	master_task->num_subtasks = 0;
	atomic_store(&master_task->num_pending_subtasks, 1);

	dynamic_array_init(0, sizeof(task_t*), &master_task->dependants);
	atomic_store(&master_task->num_pending_dependencies, 0);

	master_task->work.function = prv_graph_master_task_function;
	master_task->work.dispose = prv_graph_master_task_dispose;
	master_task->work.context = graph;

	daggle_task_add_subgraph(master_task, tasks.data, tasks.length);
	dynamic_array_destroy(&tasks);

	*out_task = master_task;

	RETURN_STATUS(DAGGLE_SUCCESS);

node_error:
	for (uint64_t i = 0; i <= tasks.length; ++i) {
		task_t** tkelem = dynamic_array_at(&tasks, i);
		task_t* tk = *tkelem;
		task_free(tk);
	}

	dynamic_array_destroy(&tasks);

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
	while (atomic_load(&t->num_pending_subtasks) > 0) { };

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
