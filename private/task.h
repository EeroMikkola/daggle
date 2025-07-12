#pragma once

#include "pthread.h"
#include "utility/dynamic_array.h"
#include "utility/thread_safe_linked_queue.h"

#include <daggle/daggle.h>

typedef struct task_callbacks_s {
	daggle_task_callback_fn start;
	daggle_task_callback_fn complete;
	daggle_task_callback_dispose_fn dispose;
	void* context;
} task_callbacks_t;

typedef struct task_s {
	task_callbacks_t callbacks;

	char* id;

	struct task_s* head; // Parent of the subgraph this is a part of
	struct task_s* tail; // Tail of this' own subgraph

	uint64_t num_subtasks; // number of subtasks (incl. sink)
	_Atomic(uint64_t) num_pending_subtasks; // this + sum of subtask progress

	dynamic_array_t dependants;
	_Atomic(uint64_t) num_pending_dependencies;
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
