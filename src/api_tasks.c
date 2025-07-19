#include "daggle/daggle.h"
#include "task.h"
#include "stdatomic.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "utility/dynamic_array.h"
#include "utility/log_macro.h"
#include "utility/return_macro.h"
#include <stdlib.h>
#include <string.h>

void
prv_sink_closure(daggle_task_h task, void* context)
{
}

// TODO: Remove this, and the one in executor_try_get_and_run_task.
// Implement a better cleanup solution.
void
prv_sink_dispose(void* context)
{
	task_t* headtask = context;
	task_free(headtask);
}

daggle_error_code_t
daggle_task_create(daggle_task_callback_fn start, daggle_task_callback_fn complete, daggle_task_callback_dispose_fn dispose, 
	void* context, char* id, daggle_task_h* out_task)
{
	task_t* task = malloc(sizeof(task_t));
	task->tail = NULL;
	task->head = NULL;

	task->num_subtasks = 0;
	atomic_store(&task->num_pending_subtasks, 1);

	dynamic_array_init(0, sizeof(task_t*), &task->dependants);
	atomic_store(&task->num_pending_dependencies, 0);

	task_callbacks_t callbacks = {
		.start = start,
		.complete = complete,
		.dispose = dispose,
		.context = context,
	};

	task->callbacks = callbacks;

	task->id = strdup(id);

	LOG_FMT_COND_DEBUG("Task create %s (%p)", task->id, task);

	*out_task = task;

	RETURN_STATUS(DAGGLE_SUCCESS);
}

// Set dependencies in the flattened representation of task graph
// Depending on a task will only depend on the task, not also it's subtasks.
daggle_error_code_t
prv_task_depend_flat(task_t* task, task_t* dependency)
{
	LOG_FMT_COND_DEBUG("Task dependency %s (%p) -> %s (%p)", task->id, task, dependency->id, dependency);

	atomic_fetch_add(&task->num_pending_dependencies, 1);
	dynamic_array_push(&dependency->dependants, &task);

	RETURN_STATUS(DAGGLE_SUCCESS);
}

// Set dependencies in the subgraph representation of task graph
// Depending on a task will depend on it and its subtasks.
daggle_error_code_t
daggle_task_depend(daggle_task_h task, daggle_task_h dependency)
{
	task_t* dependency_impl = dependency;
	if (dependency_impl->tail && dependency_impl->tail != dependency_impl
		&& dependency_impl->tail->head == dependency_impl) {
		dependency_impl = dependency_impl->tail;
	}

	task_t* task_impl = task;

	// TODO: Enable this. Has not been tested, but might fix a bug if task has subtask
	//if (task_impl->tail && task_impl->tail != task_impl
	//	&& task_impl->tail->head == task_impl) {
	//	task_impl = task_impl->tail;
	//}

	RETURN_STATUS(prv_task_depend_flat(task_impl, dependency_impl));
}

// TODO: There should be two options: parallel and serial
// 1) When two subgraphs are added, they exist in parallel (depends on subgraph head, dependant of sink)
// 2) When the second subgraph is added, it is executed after the first (depends on task sink, dependant of another sink)
// The current behavior is serial.
daggle_error_code_t
daggle_task_add_subgraph(daggle_task_h task, daggle_task_h* tasks,
	uint64_t num_tasks)
{
	ASSERT_PARAMETER(task);
	ASSERT_PARAMETER(tasks);

	// TODO: Check for cycles

	task_t* task_impl = task;

	LOG_FMT_COND_DEBUG("Task subgraph %s (%p) n:%llu", task_impl->id, task_impl, num_tasks);

	task_t* tail = NULL;
	if (task_impl->tail == NULL) {
		char* id = calloc(strlen(task_impl->id) + 6, sizeof(char));
		strcpy(id, task_impl->id);
		strcat(id, ".tail");

		daggle_task_create(prv_sink_closure, NULL, prv_sink_dispose, task_impl, id, (void*)&tail);

		free(id);

		// Set the head (the task the tail is a subtask of) to the task.
		tail->head = task_impl;

		// As task does not have a tail, it should not have subtasks.
		ASSERT_TRUE(task_impl->num_subtasks == 0, "task_impl->num_subtasks == 0");

		// Add tail to subtasks/
		task_impl->num_subtasks += 1;
		atomic_fetch_add(&task_impl->num_pending_subtasks, 1);

		// Swap tail and task dependants.
		dynamic_array_t temp = tail->dependants;
		tail->dependants = task_impl->dependants;
		task_impl->dependants = temp;

		prv_task_depend_flat(tail, task_impl);

		// Set the tail.
		task_impl->tail = tail;
	} else {
		tail = task_impl->tail;
	}

	task_impl->num_subtasks += num_tasks;
	atomic_fetch_add(&task_impl->num_pending_subtasks, num_tasks);

	for (uint64_t i = 0; i < num_tasks; ++i) {
		task_t* subtask = tasks[i];

		// Make tail a dependant of every task.
		// Only tasks without dependants have to be the dependencies of the
		// sink.
		if (subtask->dependants.length == 0) {
			prv_task_depend_flat(tail, subtask);
		}

		// Make the original task a dependency of every task.
		// Only tasks without dependencies have to be dependants of the
		// original task.
		if (atomic_load(&subtask->num_pending_dependencies) == 0) {
			prv_task_depend_flat(subtask, task_impl);
		}

		subtask->head = task_impl;
	}

	RETURN_STATUS(DAGGLE_SUCCESS);
}
