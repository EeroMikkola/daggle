#include "executor.h"

#include "stdatomic.h"
#include "stdio.h"
#include "stdlib.h"
#include "utility/return_macro.h"

#define NUM_THREADS 2

void
task_free(task_t* task)
{
	void_closure_dispose(&task->work);
	dynamic_array_destroy(&task->dependants);
	free(task);
}

void
prv_propagate_subtask_progress(task_t* task)
{
	if (atomic_fetch_sub(&task->num_pending_subtasks, 1) > 1) {
		return;
	}

	if (!task->head) {
		return;
	}

	prv_propagate_subtask_progress(task->head);
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

typedef struct prv_worker_ctx {
	executor_t* executor;
	uint64_t id;
} prv_worker_ctx_t;

void*
prv_worker_thread(void* context)
{
	prv_worker_ctx_t* context_impl = context;
	executor_t* executor = context_impl->executor;

	while (!executor->halt) {
		task_t* task;
		ts_llist_queue_dequeue(&executor->queue, &executor->halt,
			(void**)&task);

		// Continue if the task is NULL (null enqueued or no tasks available)
		if (!task) {
			continue;
		}

		// Call the task work function.
		void_closure_call(&task->work);

		prv_propagate_subtask_progress(task);
		prv_propagate_dependency_progress(task, executor);

		// If the task has a subgraph, the task is freed in the tail dispose.
		if (!task->tail) {
			// TODO: Come up with a more descriptive name for task_free:
			// it calls the dispose function, which is essentially used to run
			// code after task finishes; not just freeing the allocated data!
			task_free(task);
		}
	}

	free(context_impl);

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
		prv_worker_ctx_t* ctx = malloc(sizeof *ctx);
		ctx->executor = executor;
		ctx->id = i;

		pthread_create(executor->workers + i, NULL, &prv_worker_thread, ctx);
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