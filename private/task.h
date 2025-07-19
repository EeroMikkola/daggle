#pragma once

#include "utility/dynamic_array.h"

#include <daggle/daggle.h>

typedef struct task_callbacks_s {
	daggle_task_callback_fn start;
	daggle_task_callback_fn complete;
	daggle_task_callback_dispose_fn dispose;
	void* context;
} task_callbacks_t;

// TODO: Separate task definition (callbacks, id, head, tail, dependencies), from the instance (dependants, pending deps/dants)

// TODO: Store declared tasks in an array. Store dynamic tasks in a list.

// TODO: rework subtasks to support static/dynamic task separation

// The static task array is immutable. No new tasks, no changes in dependencies.

// Static task subtasks -> create sink.

// Dynamic task subtasks -> create in dynamic tasks, move complete call to subtask finish.
// sink would be created in dynamic array. Sink calls the static on complete.
// Currently support only one subtask; call it set_subtask.

// Approach would allow changes (predictable and reversable) in the dependency structure.

typedef struct task_execution_s {
	_Atomic(uint64_t) num_pending_subtasks; // this + sum of subtask progress
	_Atomic(uint64_t) num_pending_dependencies;
} task_execution_t;

typedef struct task_s {
	task_callbacks_t callbacks;

	char* id;

	struct task_s* head; // Parent of the subgraph this is a part of
	struct task_s* tail; // Tail of this' own subgraph

	uint64_t num_subtasks; // number of subtasks (incl. sink)
	dynamic_array_t dependants;

	task_execution_t execution;
} task_t;

void
task_free(task_t* task);

// Add callbacks to call before the original. Use to add multiple callbacks to a task.
void
task_add_callback_wrapper(task_t* task, daggle_task_callback_fn start, 
	daggle_task_callback_fn complete, daggle_task_callback_dispose_fn dispose, 
	void* context);

// Remove the wrapper. Assumes the task is wrapped; there is no validation mechanism in place.
void
task_remove_callback_wrapper(task_t* task);

void
task_run(task_t* task, void(*handle_dependant_ready)(void* ctx, task_t* task), void* callback_context);