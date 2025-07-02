#pragma once

#include "pthread.h"
#include "utility/llist_queue.h"

#include <daggle/daggle.h>

typedef struct ts_llist_queue_s {
	llist_queue_t queue;
	pthread_mutex_t lock;
	pthread_cond_t condition;
} ts_llist_queue_t;

void
ts_llist_queue_init(ts_llist_queue_t* queue);

void
ts_llist_queue_destroy(ts_llist_queue_t* queue);

daggle_error_code_t
ts_llist_queue_enqueue(ts_llist_queue_t* queue, void* payload);

void
ts_llist_queue_dequeue(ts_llist_queue_t* queue, volatile bool* interrupt,
	void** out_payload);
