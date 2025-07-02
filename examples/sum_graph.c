#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include <daggle/daggle.h>

#define PLUGIN_ROOT_PATH "plugins/"
#define CORE_PATH PLUGIN_ROOT_PATH "/core/plugin/core.daggle"

int
main(void)
{
	daggle_plugin_source_t core_source;

	daggle_plugin_source_create_from_file(CORE_PATH, &core_source);

	daggle_plugin_source_t* plugins[] = { &core_source };

	daggle_instance_h instance;
	daggle_instance_create(plugins, 1, &instance);

	daggle_graph_h graph;
	daggle_graph_create(instance, &graph);

	daggle_node_h input_a;
	daggle_graph_add_node(graph, "input", &input_a);

	daggle_port_h input_a_value;
	daggle_port_h input_a_result;

	daggle_node_get_port_by_name(input_a, "value", &input_a_value);
	daggle_node_get_port_by_name(input_a, "result", &input_a_result);

	daggle_node_h input_b;
	daggle_graph_add_node(graph, "input", &input_b);

	daggle_port_h input_b_value;
	daggle_port_h input_b_result;

	daggle_node_get_port_by_name(input_b, "value", &input_b_value);
	daggle_node_get_port_by_name(input_b, "result", &input_b_result);

	daggle_node_h math;
	daggle_graph_add_node(graph, "math", &math);

	daggle_port_h math_first;
	daggle_port_h math_second;
	daggle_port_h math_operation;
	daggle_port_h math_result;

	daggle_node_get_port_by_name(math, "first", &math_first);
	daggle_node_get_port_by_name(math, "second", &math_second);
	daggle_node_get_port_by_name(math, "operation", &math_operation);
	daggle_node_get_port_by_name(math, "result", &math_result);

	daggle_node_h output;
	daggle_graph_add_node(graph, "output", &output);

	daggle_port_h output_value;
	daggle_port_h output_message;

	daggle_node_get_port_by_name(output, "value", &output_value);
	daggle_node_get_port_by_name(output, "message", &output_message);

	int32_t* input_a_value_data = malloc(sizeof *input_a_value_data);
	*input_a_value_data = 2;
	daggle_port_set_value(input_a_value, "int", input_a_value_data);

	int32_t* input_b_value_data = malloc(sizeof *input_b_value_data);
	*input_b_value_data = 3;
	daggle_port_set_value(input_b_value, "int", input_b_value_data);

	int32_t* math_second_data = malloc(sizeof *math_second_data);
	*math_second_data = 5;
	daggle_port_set_value(math_second, "int", math_second_data);

	int32_t* math_operation_data = malloc(sizeof *math_operation_data);
	*math_operation_data = 2;
	daggle_port_set_value(math_operation, "int", math_operation_data);

	char* output_message_data = strdup("Custom Output: ");
	daggle_port_set_value(output_message, "string", output_message_data);

	daggle_node_get_port_by_name(input_a, "result", &input_a_result);
	daggle_node_get_port_by_name(math, "first", &math_first);
	daggle_node_get_port_by_name(input_b, "result", &input_b_result);
	daggle_node_get_port_by_name(math, "second", &math_second);
	daggle_node_get_port_by_name(math, "result", &math_result);
	daggle_node_get_port_by_name(output, "value", &output_value);

	daggle_port_connect(input_a_result, math_first);
	daggle_port_connect(input_b_result, math_second);
	daggle_port_connect(math_result, output_value);

	// Modify the value of a connected port (should warn).
	int32_t* test_data = malloc(sizeof *test_data);
	*test_data = 2;
	daggle_port_set_value(math_first, "int", test_data);

	daggle_graph_execute(instance, graph);

	unsigned char* bin;
	uint64_t len;
	daggle_graph_serialize(graph, &bin, &len);
	daggle_graph_free(graph);

	daggle_graph_h graph2;
	daggle_graph_deserialize(instance, bin, &graph2);
	free(bin);

	daggle_graph_execute(instance, graph2);

	daggle_instance_free(instance);

	printf("Program exit\n");

	return 0;
}
