#include "utility/llist_queue.h"

#include "stdlib.h"
#include "utility/return_macro.h"

void
llist_queue_init(llist_queue_t* queue)
{
	ASSERT_PARAMETER(queue);

	queue->head = NULL;
	queue->tail = NULL;
}

void
llist_queue_destroy(llist_queue_t* queue)
{
	ASSERT_PARAMETER(queue);

	if (!queue->head) {
		return;
	}

	// Free the remaining elements in the queue.
	llist_node_t* current = queue->head;
	llist_node_t* next = NULL;
	while (current) {
		next = current->next;
		free(current);
		current = next;
	}
}

daggle_error_code_t
llist_queue_enqueue(llist_queue_t* queue, void* payload)
{
	ASSERT_PARAMETER(queue);

	llist_node_t* node = malloc(sizeof *node);
	REQUIRE_ALLOCATION_DAGGLE_SUCCESSFUL(node);

	node->data = payload;
	node->next = NULL;

	if (queue->tail) {
		queue->tail->next = node;
	} else {
		queue->head = node;
	}

	queue->tail = node;

	RETURN_STATUS(DAGGLE_SUCCESS);
}

void
llist_queue_dequeue(llist_queue_t* queue, void** out_payload)
{
	ASSERT_PARAMETER(queue);
	ASSERT_OUTPUT_PARAMETER(out_payload);

	if (!queue->head) {
		*out_payload = NULL;
		return;
	}

	llist_node_t* node = queue->head;
	queue->head = node->next;
	if (!queue->head) {
		queue->tail = NULL;
	}

	*out_payload = node->data;

	free(node);
}
