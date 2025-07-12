#include "executor.h"
#include "task.h"

#include "stdatomic.h"
#include "stdio.h"
#include "stdlib.h"
#include "utility/log_macro.h"
#include "utility/return_macro.h"
#include "stdint.h"

#define NUM_THREADS 1

void
prv_propagate_subtask_progress(task_t* task)
{
	task_t* head = task->head;
	LOG_FMT_COND_DEBUG("Task progress %s (%p) %llu:%llu p:%s (%p)", task->id, task, atomic_load(&task->num_pending_subtasks)-1, task->num_subtasks, (head ? head->id : "null"), head);

	ASSERT_TRUE(atomic_load(&task->num_pending_subtasks) != 0, "Subgraph completion called multiple times");

	if (atomic_fetch_sub(&task->num_pending_subtasks, 1) > 1) {
		return;
	}

	if(task->callbacks.complete) {
		LOG_FMT_COND_DEBUG("Task complete %s (%p)", task->id, task);
		task->callbacks.complete(task, task->callbacks.context);
	}

	if (!head) {
		return;
	}

	prv_propagate_subtask_progress(head);
}

void
prv_propagate_dependency_progress(task_t* task, executor_t* executor)
{
	for (uint64_t i = 0; i < task->dependants.length; ++i) {
		task_t** task_element = dynamic_array_at(&task->dependants, i);
		task_t* task = *task_element;

		if (atomic_fetch_sub(&task->num_pending_dependencies, 1) == 1) {
			ts_llist_queue_enqueue(&executor->queue, task);
		}
	}
}

void
executor_try_get_and_run_task(executor_t* executor) {
	task_t* task;
	ts_llist_queue_dequeue(&executor->queue, &executor->halt,
		(void**)&task);

	// Return if task is null. Happens if NULL is enqueued, if no tasks are
	// available, or if execution was halted.
	if (!task) {
		return;
	}

	LOG_FMT_COND_DEBUG("Task run %s (%p)", task->id, task);

	// Call the task work function.
	if(task->callbacks.start) {
		task->callbacks.start(task, task->callbacks.context);
	}

	prv_propagate_dependency_progress(task, executor);
	prv_propagate_subtask_progress(task);

	// If the task has a subgraph, the task is freed in the tail dispose.
	if (!task->tail) {
		task_free(task);
	}
}

void*
prv_worker_thread(void* context)
{
	executor_t* executor = context;

	while (!executor->halt) {
		executor_try_get_and_run_task(executor);
	}

	return NULL;
}

daggle_error_code_t
executor_init(executor_t* executor)
{
	ASSERT_PARAMETER(executor);

	ts_llist_queue_init(&executor->queue);

	executor->halt = false;
	executor->workers = malloc(sizeof(pthread_t) * NUM_THREADS);

	for (uint64_t i = 0; i < NUM_THREADS; ++i) {
		pthread_create(
			executor->workers + i, NULL, &prv_worker_thread, executor);
	}

	RETURN_STATUS(DAGGLE_SUCCESS);
}

void
executor_destroy(executor_t* executor)
{
	ASSERT_PARAMETER(executor);

	executor->halt = true;
	pthread_cond_broadcast(&executor->queue.condition);

	for (uint64_t i = 0; i < NUM_THREADS; ++i) {
		pthread_cancel(executor->workers[i]);
		pthread_join(executor->workers[i], NULL);
	}

	ts_llist_queue_destroy(&executor->queue);

	free(executor->workers);
}