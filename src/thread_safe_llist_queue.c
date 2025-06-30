#include "pthread.h"
#include "stdlib.h"
#include "utility/return_macro.h"
#include "utility/thread_safe_linked_queue.h"

void
ts_llist_queue_init(
	ts_llist_queue_t* queue)
{
	ASSERT_PARAMETER(queue);

	// Initialize the underlying queue
	llist_queue_init(&queue->queue);

	// Initialize thread-safety -related stuff.
	pthread_mutex_init(&queue->lock, NULL);
	pthread_cond_init(&queue->condition, NULL);
}

void
ts_llist_queue_destroy(
	ts_llist_queue_t* queue)
{
	ASSERT_PARAMETER(queue);

	llist_queue_destroy(&queue->queue);
}

daggle_error_code_t
ts_llist_queue_enqueue(
	ts_llist_queue_t* queue, void* payload)
{
	ASSERT_PARAMETER(queue);

	pthread_mutex_lock(&queue->lock);

	// Could be improved by moving the node allocation outside of the function.
	daggle_error_code_t error = llist_queue_enqueue(&queue->queue, payload);
	GOTO_IF_ERROR(error, enqueue_error);

	pthread_cond_signal(&queue->condition);
	pthread_mutex_unlock(&queue->lock);

	RETURN_STATUS(DAGGLE_SUCCESS);

enqueue_error:
	pthread_mutex_unlock(&queue->lock);
	RETURN_STATUS(error);
}

void
ts_llist_queue_dequeue(
	ts_llist_queue_t* queue, volatile bool* interrupt, void** out_payload)
{
	ASSERT_PARAMETER(queue);
	ASSERT_PARAMETER(interrupt);
	ASSERT_OUTPUT_PARAMETER(out_payload);

	pthread_mutex_lock(&queue->lock);

	// Thread will wait untill the condition variable is signaled.
	// This happens when:
	// a) new task is available.
	// b) the execution is interrupted.
	// Interruption happens when the execution finishes, or cancel is called.
	while(!queue->queue.head && !*interrupt) {
		pthread_cond_wait(&queue->condition, &queue->lock);
	}

	if(*interrupt) {
		pthread_mutex_unlock(&queue->lock);
		*out_payload = NULL;
		return;
	}

	void* payload;
	llist_queue_dequeue(&queue->queue, &payload);
	*out_payload = payload;

	pthread_mutex_unlock(&queue->lock);
}
