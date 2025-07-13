#include "executor.h"
#include "task.h"

#include "stdatomic.h"
#include "stdio.h"
#include "stdlib.h"
#include "utility/log_macro.h"
#include "utility/return_macro.h"
#include "stdint.h"
#include "utility/thread_safe_linked_queue.h"

#define NUM_THREADS 1

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

	// TODO: Consider designing something better. 
	task_run(task, (void*)ts_llist_queue_enqueue, &executor->queue);

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