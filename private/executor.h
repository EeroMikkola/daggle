#pragma once

#include "pthread.h"
#include "utility/closure.h"
#include "utility/dynamic_array.h"
#include "utility/thread_safe_linked_queue.h"

#include <daggle/daggle.h>

typedef struct task_s {
	void_closure_t work;

	struct task_s* head; // Parent of the subgraph this is a part of
	struct task_s* tail; // Tail of this' own subgraph

	uint64_t num_subtasks; // number of subtasks (incl. sink)
	_Atomic(uint64_t) num_pending_subtasks; // this + sum of subtask progress

	dynamic_array_t dependants;
	_Atomic(uint64_t) num_pending_dependencies;
} task_t;

typedef struct executor {
	ts_llist_queue_t queue;
	pthread_t* workers;
	volatile bool halt;
} executor_t;

void
task_free(task_t* task);

daggle_error_code_t
executor_init(executor_t* executor);

void
executor_destroy(executor_t* executor);