#include "executor.h"
#include "node.h"
#include "stdatomic.h"
#include "stdio.h"
#include "stdlib.h"
#include "utility/return_macro.h"

void
prv_sink_closure(
	void* context)
{ }

void
prv_sink_dispose(
	void* context)
{
	task_free((task_t*)context);
}

typedef struct prv_node_task_wrapper_ctx {
	task_t* task;
	daggle_node_task_fn function;
	daggle_node_task_dispose_fn dispose;
	void* context;
} prv_node_task_wrapper_ctx_t;

void
prv_node_task_wrapper_function(
	void* context)
{
	ASSERT_NOT_NULL(context, "context is null");

	prv_node_task_wrapper_ctx_t* context_impl = context;

	context_impl->function(context_impl->task, context_impl->context);
}

void
prv_node_task_wrapper_dispose(
	void* context)
{
	ASSERT_NOT_NULL(context, "context is null");

	prv_node_task_wrapper_ctx_t* context_impl = context;

	if(context_impl->dispose) {
		context_impl->dispose(context_impl->context);
	}

	free(context_impl);
}

daggle_error_code_t
daggle_task_create(
	daggle_node_task_fn work,
	daggle_node_task_dispose_fn dispose,
	void* context,
	char* id,
	daggle_task_h* out_task)
{
	task_t* task = malloc(sizeof(task_t));
	task->tail = NULL;
	task->head = NULL;

	task->num_subtasks = 0;
	atomic_store(&task->num_pending_subtasks, 1);

	dynamic_array_init(0, sizeof(task_t*), &task->dependants);
	atomic_store(&task->num_pending_dependencies, 0);

	prv_node_task_wrapper_ctx_t* ctx = malloc(sizeof *ctx);
	ctx->context = context;
	ctx->function = work;
	ctx->dispose = dispose;
	ctx->task = task;

	task->work.function = prv_node_task_wrapper_function;
	task->work.dispose = prv_node_task_wrapper_dispose;
	task->work.context = ctx;

	*out_task = task;

	RETURN_STATUS(DAGGLE_SUCCESS);
}

// Set dependencies in the flattened representation of task graph
// Depending on a task will only depend on the task, not also it's subtasks.
daggle_error_code_t
prv_task_depend_flat(
	task_t* task, task_t* dependency)
{
	atomic_fetch_add(&task->num_pending_dependencies, 1);
	dynamic_array_push(&dependency->dependants, &task);

	RETURN_STATUS(DAGGLE_SUCCESS);
}

// Set dependencies in the subgraph representation of task graph
// Depending on a task will depend on it and its subtasks.
daggle_error_code_t
daggle_task_depend(
	daggle_task_h task, daggle_task_h dependency)
{
	task_t* dependency_impl = dependency;
	if(dependency_impl->tail && dependency_impl->tail != dependency_impl
		&& dependency_impl->tail->head == dependency_impl) {
		dependency_impl = dependency_impl->tail;
	}

	RETURN_STATUS(prv_task_depend_flat(task, dependency_impl));
}

daggle_error_code_t
daggle_task_add_subgraph(
	daggle_task_h task, daggle_task_h* tasks, uint64_t num_tasks)
{
	ASSERT_PARAMETER(task); 
	ASSERT_PARAMETER(tasks);

	// TODO: Check for cycles

	task_t* task_impl = task;
	daggle_task_h* tasks_impl = tasks;

	task_t* tail = NULL;
	if(task_impl->tail == NULL) {
		tail = malloc(sizeof(task_t));
		tail->tail = NULL;
		tail->head = task_impl;

		tail->num_subtasks = 0;
		atomic_store(&tail->num_pending_subtasks, 1);

		tail->work.function = prv_sink_closure;
		tail->work.dispose
			= prv_sink_dispose; // The tail will free the parent task.
		tail->work.context = task_impl;
		atomic_store(&tail->num_pending_dependencies, 0);

		// Sink is a subtask
		task_impl->num_subtasks = 1;

		// Transfer dependants to sink.
		tail->dependants = task_impl->dependants;
		dynamic_array_init(0, sizeof(task_t*), &task_impl->dependants);

		// Set the tail.
		task_impl->tail = tail;
	} else {
		tail = task_impl->tail;
	}

	task_impl->num_subtasks += num_tasks;
	atomic_fetch_add(&task_impl->num_pending_subtasks, num_tasks);
	for(uint64_t i = 0; i < num_tasks; ++i) {
		task_t* subtask = tasks[i];
	}

	for(uint64_t i = 0; i < num_tasks; ++i) {
		task_t* subtask = tasks[i];

		// Make tail a dependant of every task.
		// Only tasks without dependants have to be the dependencies of the
		// sink.
		if(subtask->dependants.length == 0) {
			prv_task_depend_flat(tail, subtask);
		}

		// Make the original task a dependency of every task.
		// Only tasks without dependencies have to be dependants of the
		// original task.
		if(atomic_load(&subtask->num_pending_dependencies) == 0) {
			prv_task_depend_flat(subtask, task_impl);
		}

		subtask->head = task_impl;
	}

	RETURN_STATUS(DAGGLE_SUCCESS);
}
