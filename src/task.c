#include "task.h"

#include "stdatomic.h"
#include "stdio.h"
#include "stdlib.h"
#include "utility/log_macro.h"
#include "utility/return_macro.h"
#include "stdint.h"


typedef struct prv_task_wrapper_ctx {
	task_callbacks_t wrapper;
	task_callbacks_t original;
} prv_task_wrapper_ctx_t;

void
prv_task_wrapper_start(daggle_task_h task, void* context)
{
	ASSERT_NOT_NULL(context, "context is null");

	prv_task_wrapper_ctx_t* ctx = context;

	if(ctx->wrapper.start) {
		ctx->wrapper.start(task, ctx->wrapper.context);
	}

	if(ctx->original.start) {
		ctx->original.start(task, ctx->original.context);
	}
}

void
prv_task_wrapper_complete(daggle_task_h task, void* context)
{
	ASSERT_NOT_NULL(context, "context is null");

	prv_task_wrapper_ctx_t* ctx = context;

	if(ctx->wrapper.complete) {
		ctx->wrapper.complete(task, ctx->wrapper.context);
	}

	if(ctx->original.complete) {
		ctx->original.complete(task, ctx->original.context);
	}
}

void
prv_task_wrapper_dispose(void* context)
{
	ASSERT_NOT_NULL(context, "context is null");

	prv_task_wrapper_ctx_t* ctx = context;

	if(ctx->wrapper.dispose) {
		ctx->wrapper.dispose(ctx->wrapper.context);
	}

	if(ctx->original.dispose) {
		ctx->original.dispose(ctx->original.context);
	}

	free(ctx);
}

void
task_free(task_t* task)
{
	LOG_FMT_COND_DEBUG("Task dispose %s (%p)", task->id, task);

	if(task->callbacks.dispose) {
		task->callbacks.dispose(task->callbacks.context);
	}
	
	dynamic_array_destroy(&task->dependants);
	free(task);
}

void
task_add_callback_wrapper(task_t* task, daggle_task_callback_fn start, 
	daggle_task_callback_fn complete, daggle_task_callback_dispose_fn dispose, 
	void* context) {

	prv_task_wrapper_ctx_t* wctx = malloc(sizeof(*wctx));

	task_callbacks_t wrapper = {
		.start = start,
		.complete = complete,
		.dispose = dispose,
		.context = context,
	};

	wctx->wrapper = wrapper;
	wctx->original = task->callbacks;

	task_callbacks_t handlers = {
		.start = prv_task_wrapper_start,
		.complete = prv_task_wrapper_complete,
		.dispose = prv_task_wrapper_dispose,
		.context = wctx,
	};

	task->callbacks = handlers;
}

void
task_remove_callback_wrapper(task_t* task) {
	ASSERT_NOT_NULL(task->callbacks.context, "context null");

	prv_task_wrapper_ctx_t* context = task->callbacks.context;

	// Dispose the wrapper
	ASSERT_NOT_NULL(task->callbacks.dispose, "missing dispose");
	context->wrapper.dispose(context->wrapper.context);

	// Replace callbacks with the original.
	task->callbacks = context->original;

	// Free the wrapper context
	free(context);
}