#pragma once

#include "utility/thread_safe_linked_queue.h"

#include <daggle/daggle.h>

typedef struct executor {
	ts_llist_queue_t queue;
	pthread_t* workers;
	volatile bool halt;
} executor_t;

daggle_error_code_t
executor_init(executor_t* executor);

void
executor_try_get_and_run_task(executor_t* executor);

void
executor_destroy(executor_t* executor);