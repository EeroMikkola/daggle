#pragma once

#include <daggle/daggle.h>

typedef struct llist_node_s {
	struct llist_node_s* next;
	void* data;
} llist_node_t;

typedef struct llist_queue_s {
	llist_node_t* head;
	llist_node_t* tail;
} llist_queue_t;

void
llist_queue_init(llist_queue_t* queue);

void
llist_queue_destroy(llist_queue_t* queue);

daggle_error_code_t
llist_queue_enqueue(llist_queue_t* queue, void* payload);

void
llist_queue_dequeue(llist_queue_t* queue, void** out_payload);
