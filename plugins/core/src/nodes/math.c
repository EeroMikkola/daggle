#include "nodes/math.h"

#include "node_utils.h"
#include "stdlib.h"
#include "types.h"

typedef struct math_context {
	daggle_node_h node;

	int32_t first;
	int32_t second;
	int32_t operation;

	int32_t result;
} math_context_t;

void
math_read_fn(daggle_task_h task, void* context)
{
	math_context_t* math_context = context;

	daggle_port_h firstPort;
	daggle_port_h secondPort;
	daggle_port_h operationPort;

	daggle_node_get_port_by_name(math_context->node, "first", &firstPort);
	daggle_node_get_port_by_name(math_context->node, "second", &secondPort);
	daggle_node_get_port_by_name(math_context->node, "operation",
		&operationPort);

	int32_t* first;
	int32_t* second;
	int32_t* operation;

	daggle_port_get_value(firstPort, (void**)&first);
	daggle_port_get_value(secondPort, (void**)&second);
	daggle_port_get_value(operationPort, (void**)&operation);

	math_context->first = *first;
	math_context->second = *second;
	math_context->operation = *operation;
}

void
math_calculate_fn(daggle_task_h task, void* context)
{
	math_context_t* math_context = context;

	int32_t res;
	switch (math_context->operation) {
	default:
	case 0:
		res = math_context->first + math_context->second;
		break;
	case 1:
		res = math_context->first - math_context->second;
		break;
	case 2:
		res = math_context->first * math_context->second;
		break;
	case 3:
		res = math_context->first / math_context->second;
		break;
	}

	math_context->result = res;
}

void
math_write_fn(daggle_task_h task, void* context)
{
	math_context_t* math_context = context;

	daggle_port_h outputPort;
	daggle_node_get_port_by_name(math_context->node, "result", &outputPort);

	int32_t* result = malloc(sizeof *result);
	*result = math_context->result;
	daggle_port_set_value(outputPort, INT_TYPE, result);
}

void
math_write_dispose(void* context)
{
	free((math_context_t*)context);
}

void
math_impl(daggle_task_h task, void* context)
{
	math_context_t* math_context = malloc(sizeof *math_context);
	math_context->node = context;

	daggle_task_h reader_task;
	daggle_task_create(math_read_fn, NULL, math_context, "math_read\0",
		&reader_task);

	daggle_task_h calculator_task;
	daggle_task_create(math_calculate_fn, NULL, math_context,
		"math_calculate\0", &calculator_task);

	daggle_task_h writer_task;
	daggle_task_create(math_write_fn, math_write_dispose, math_context,
		"math_write\0", &writer_task);

	daggle_task_depend(writer_task, calculator_task);
	daggle_task_depend(calculator_task, reader_task);

	daggle_task_h subgraph[3] = { reader_task, calculator_task, writer_task };

	daggle_task_add_subgraph(task, (daggle_task_h*)&subgraph, 3);
}

DEFAULT_VALUE_GENERATOR(math_gdv_first, int32_t, 5, INT_TYPE)
DEFAULT_VALUE_GENERATOR(math_gdv_second, int32_t, 2, INT_TYPE)
DEFAULT_VALUE_GENERATOR(math_gdv_operation, int32_t, 0, INT_TYPE)

void
math(daggle_node_h handle)
{
	daggle_node_declare_input(handle, "first", DAGGLE_INPUT_IMMUTABLE_REFERENCE,
		math_gdv_first);
	daggle_node_declare_input(handle, "second",
		DAGGLE_INPUT_IMMUTABLE_REFERENCE, math_gdv_second);
	daggle_node_declare_parameter(handle, "operation", math_gdv_operation);
	daggle_node_declare_output(handle, "result");
	daggle_node_declare_task(handle, math_impl);
}
