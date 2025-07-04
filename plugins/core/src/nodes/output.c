#include "nodes/output.h"

#include "node_utils.h"
#include "stdio.h"
#include "stdlib.h"
#include "types.h"

void
output_impl(daggle_task_h task, void* context)
{
	daggle_node_h handle = context;

	daggle_port_h messagePort;
	daggle_port_h valuePort;

	daggle_node_get_port_by_name(handle, "message", &messagePort);
	daggle_node_get_port_by_name(handle, "value", &valuePort);

	void* message;
	daggle_port_get_value(messagePort, &message);

	void* value;
	daggle_port_get_value(valuePort, &value);

	char* msg = (char*)message;
	int32_t val = *(int32_t*)value;

	printf("%s%i\n", msg, val);
}

DEFAULT_VALUE_GENERATOR(output_gdv_value, int32_t, 1, INT_TYPE)

bool
output_gdv_message(void** out_data, const char** out_type)
{
	char* data = malloc(sizeof(char) * 9);
	data[0] = 'O';
	data[1] = 'u';
	data[2] = 't';
	data[3] = 'p';
	data[4] = 'u';
	data[5] = 't';
	data[6] = ':';
	data[7] = ' ';
	data[8] = '\0';

	*out_data = data;
	*out_type = STRING_TYPE;
	return true;
}

void
output(daggle_node_h handle)
{
	daggle_node_declare_input(handle, "value", DAGGLE_INPUT_BEHAVIOR_REFERENCE,
		output_gdv_value);
	daggle_node_declare_input(handle, "message",
		DAGGLE_INPUT_BEHAVIOR_REFERENCE, output_gdv_message);
	daggle_node_declare_task(handle, output_impl);
}
