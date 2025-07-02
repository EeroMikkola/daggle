#include "utility/closure.h"

#include "utility/return_macro.h"

void
void_closure_call(void_closure_t* closure)
{
	ASSERT_PARAMETER(closure);
	ASSERT_NOT_NULL(closure->function, "Closure must have a function");

	closure->function(closure->context);
}

void
void_closure_dispose(void_closure_t* closure)
{
	ASSERT_PARAMETER(closure);

	if (!closure->dispose) {
		return;
	}

	closure->dispose(closure->context);
}
