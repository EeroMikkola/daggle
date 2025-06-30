#include "nodes/input.h"

#include "node_utils.h"
#include "stdlib.h"
#include "types.h"

void
input_impl(
	daggle_task_h task, void* context)
{
	daggle_node_h handle = context;

	daggle_port_h value_parameter;
	daggle_node_get_port_by_name(handle, "value", &value_parameter);

	daggle_port_h result_output;
	daggle_node_get_port_by_name(handle, "result", &result_output);

	const char* type;
	daggle_port_get_value_data_type(value_parameter, &type);

	void* value;
	daggle_port_get_value(value_parameter, &value);

	daggle_port_set_value(result_output, type, value);
}

DEFAULT_VALUE_GENERATOR(
	input_gdv_value, int32_t, 1, INT_TYPE)

void
input(
	daggle_node_h handle)
{
	daggle_node_declare_parameter(handle, "value", input_gdv_value);
	daggle_node_declare_output(handle, "result");
	daggle_node_declare_task(handle, input_impl);
}
