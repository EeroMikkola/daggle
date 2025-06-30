#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include <daggle/daggle.h>

#define PLUGIN_ROOT_PATH "plugins/"
#define CORE_PATH PLUGIN_ROOT_PATH "/core/plugin/core.daggle"
#define GRAPH_PATH PLUGIN_ROOT_PATH "/graph/graph.daggle"

void
generate_graph(daggle_instance_h instance, daggle_graph_h* out_graph)
{
	daggle_graph_h graph;
	daggle_graph_create(instance, &graph);

	daggle_node_h input;
	daggle_node_h output;

	daggle_graph_add_node(graph, "input_bridge", &input);
	daggle_graph_add_node(graph, "output_bridge", &output);

	daggle_port_h input_value;
	daggle_port_h output_value;

	daggle_node_get_port_by_name(input, "value", &input_value);
	daggle_node_get_port_by_name(output, "value", &output_value);

	daggle_port_connect(input_value, output_value);

	daggle_port_h input_name;
	daggle_port_h output_name;

	daggle_node_get_port_by_name(input, "name", &input_name);
	daggle_node_get_port_by_name(output, "name", &output_name);

	char* input_value_name = strdup("bridged_input");
	char* output_value_name = strdup("bridged_output");

	daggle_port_set_value(input_name, "string", input_value_name);
	daggle_port_set_value(output_name, "string", output_value_name);

	*out_graph = graph;
}

int
main(void)
{
	daggle_plugin_source_t core_source;
	daggle_plugin_source_t graph_source;

	daggle_plugin_source_create_from_file(CORE_PATH, &core_source);
	daggle_plugin_source_create_from_file(GRAPH_PATH, &graph_source);

	daggle_plugin_source_t* plugins[] = {&core_source, &graph_source};

	daggle_instance_h instance;
	daggle_instance_create(plugins, 2, &instance);

	daggle_graph_h graph;
	daggle_graph_create(instance, &graph);

	daggle_node_h invoker;
	daggle_graph_add_node(graph, "graph_invoker", &invoker);

	daggle_port_h graph_port;

	daggle_node_get_port_by_name(invoker, "graph", &graph_port);

	daggle_graph_h generated;
	generate_graph(instance, &generated);

	daggle_port_set_value(graph_port, "graph_object", generated);

	daggle_port_h input_value;
	daggle_port_h output_value;

	daggle_node_get_port_by_name(invoker, "bridged_input", &input_value);
	daggle_node_get_port_by_name(invoker, "bridged_output", &output_value);

	int32_t* data = malloc(sizeof *data);
	*data = 5;
	daggle_port_set_value(input_value, "int", data);

	daggle_graph_execute(instance, graph);

	int32_t* out_data;
	daggle_port_get_value(output_value, (void**)&out_data);
	printf("%zu -> %zu\n", *data, *out_data);

	daggle_graph_free(graph);
	daggle_instance_free(instance);

	printf("Program exit\n");

	return 0;
}
