#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include <daggle/daggle.h>

#ifdef _WIN32
#define PLUGIN_API __declspec(dllexport)
#else
#define PLUGIN_API __attribute__((visibility("default")))
#endif

#define DEFAULT_VALUE_GENERATOR(fname, type, val, key)                         \
	bool fname(void** out_data, const char** out_type)                         \
	{                                                                          \
		type* data = malloc(sizeof *data);                                     \
		*data = val;                                                           \
		*out_data = data;                                                      \
		*out_type = key;                                                       \
		return true;                                                           \
	}

void
clone_graph_object(daggle_instance_h instance, const void* data, void** target)
{
	daggle_graph_h graph = (void*)data;

	unsigned char* bin;
	uint64_t len;
	daggle_graph_serialize(graph, &bin, &len);

	daggle_instance_h daggle;
	daggle_graph_get_daggle(graph, &daggle);

	daggle_graph_deserialize(daggle, bin, (void**)target);
}

void
free_graph_object(daggle_instance_h instance, void* data)
{
	daggle_graph_h graph = (void*)data;
	daggle_graph_free(graph);
}

void
serialize_graph_object(daggle_instance_h instance, const void* data,
	unsigned char** out_buf, uint64_t* out_len)
{
	daggle_graph_h graph = (void*)data;
	daggle_graph_serialize(graph, out_buf, out_len);
}

void
deserialize_graph_object(daggle_instance_h instance, const unsigned char* bin,
	uint64_t len, void** target)
{
	daggle_graph_deserialize(instance, bin, (void**)target);
}

bool
bridge_name_default_value(void** out_data, const char** out_type)
{
	char* data = malloc(sizeof(char) * 1);
	data[0] = '\0';
	*out_data = data;
	*out_type = "string";
	return true;
}

bool
null_default_value(void** out_data, const char** out_type)
{
	*out_data = NULL;
	*out_type = "bytes";
	return true;
}

void
input_bridge_impl(daggle_task_h task, void* context)
{
	daggle_port_h in;
	daggle_port_h out;

	daggle_node_get_port_by_name(context, "_bridge", &in);
	daggle_node_get_port_by_name(context, "value", &out);

	const char* in_type = NULL;
	void* in_value = NULL;

	daggle_port_get_value_data_type(in, &in_type);
	daggle_port_get_value(in, &in_value);

	daggle_port_set_value(out, in_type, in_value);
}

void
output_bridge_impl(daggle_task_h task, void* context)
{
	daggle_port_h in;
	daggle_port_h out;

	daggle_node_get_port_by_name(context, "value", &in);
	daggle_node_get_port_by_name(context, "_bridge", &out);

	const char* in_type = NULL;
	void* in_value = NULL;

	daggle_port_get_value_data_type(in, &in_type);
	daggle_port_get_value(in, &in_value);

	daggle_port_set_value(out, in_type, in_value);
}

void
input_bridge(daggle_node_h handle)
{
	daggle_node_declare_parameter(handle, "name", bridge_name_default_value);
	daggle_node_declare_output(handle, "value");

	daggle_node_declare_input(handle, "_bridge", DAGGLE_INPUT_MUTABLE_COPY,
		null_default_value);

	daggle_node_declare_task(handle, input_bridge_impl);
}

void
output_bridge(daggle_node_h handle)
{
	daggle_node_declare_parameter(handle, "name", bridge_name_default_value);
	daggle_node_declare_input(handle, "value", DAGGLE_INPUT_MUTABLE_COPY,
		null_default_value);

	daggle_node_declare_output(handle, "_bridge");

	daggle_node_declare_task(handle, output_bridge_impl);
}

typedef struct graph_invoker_context_s {
	daggle_instance_h daggle;
	daggle_node_h handle;
	daggle_graph_h graph;
} graph_invoker_context_t;

void
generate_graph(graph_invoker_context_t* ctx)
{
	daggle_graph_h graph;
	daggle_graph_create(ctx->daggle, &graph);

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

	ctx->graph = graph;
}

void
graph_invoker_context_dispose(void* context)
{
	graph_invoker_context_t* ctx = context;

	if (ctx->graph) {
		// graph_free(ctx->graph);
	}

	free(ctx);
}

void
bridge_ports(daggle_port_h from, daggle_port_h to)
{
	const char* type = NULL;
	void* data = NULL;

	daggle_port_get_value_data_type(from, &type);
	daggle_port_get_value(from, &data);

	daggle_port_set_value(to, type, data);
}

void
invoker_do_bridge(graph_invoker_context_t* ctx, bool is_write)
{
	daggle_graph_h graph = ctx->graph;

	uint64_t counter = 0;
	daggle_node_h node = NULL;
	while (true) {
		daggle_graph_get_node_by_index(graph, counter++, &node);

		if (!node) {
			break;
		}

		const char* type = NULL;
		daggle_node_get_type(node, &type);

		if (strcmp(type, is_write ? "output_bridge" : "input_bridge") != 0) {
			continue;
		}

		daggle_port_h name_port;
		daggle_node_get_port_by_name(node, "name", &name_port);

		char* name_value;
		daggle_port_get_value(name_port, (void**)(&name_value));

		daggle_port_h bridge_value_port;
		daggle_node_get_port_by_name(node, "_bridge", &bridge_value_port);

		daggle_port_h invoker_value_port;
		daggle_node_get_port_by_name(ctx->handle, name_value,
			&invoker_value_port);

		if (is_write) {
			bridge_ports(bridge_value_port, invoker_value_port);
		} else {
			bridge_ports(invoker_value_port, bridge_value_port);
		}
	}
}

void
invoker_write_task(daggle_task_h task, void* context)
{
	invoker_do_bridge(context, true);
}

void
invoker_read_task(daggle_task_h task, void* context)
{
	invoker_do_bridge(context, false);
}

void
graph_invoker_impl(daggle_task_h task, void* context)
{
	graph_invoker_context_t* ctx = context;
	daggle_graph_h graph = ctx->graph;

	daggle_task_h graph_task;
	daggle_graph_taskify(graph, &graph_task);

	daggle_task_h read_task;
	daggle_task_create(invoker_read_task, NULL, ctx, "read", &read_task);

	daggle_task_h write_task;
	daggle_task_create(invoker_write_task, NULL, ctx, "write", &write_task);

	daggle_task_depend(graph_task, read_task);
	daggle_task_depend(write_task, graph_task);

	daggle_task_h tasks[3] = { read_task, graph_task, write_task };

	daggle_task_add_subgraph(task, tasks, 3);
}

void
graph_invoker_declare_graph_bridges(daggle_node_h handle, daggle_graph_h graph)
{
	daggle_node_h subnode = NULL;
	uint64_t counter = 0;
	while (true) {
		daggle_graph_get_node_by_index(graph, counter++, &subnode);

		if (!subnode) {
			break;
		}

		const char* type = NULL;
		daggle_node_get_type(subnode, &type);

		bool is_input = strcmp(type, "input_bridge") == 0;
		bool is_output = !is_input && strcmp(type, "output_bridge") == 0;

		if (!is_input && !is_output) {
			continue;
		}

		daggle_port_h name_port;
		daggle_node_get_port_by_name(subnode, "name", &name_port);

		char* name_value;
		daggle_port_get_value(name_port, (void**)(&name_value));

		if (is_input) {
			daggle_node_declare_input(handle, name_value,
				DAGGLE_INPUT_MUTABLE_COPY, null_default_value);
		} else {
			daggle_node_declare_output(handle, name_value);
		}
	}
}

void
graph_invoker(daggle_node_h handle)
{
	daggle_node_declare_parameter(handle, "graph",
		null_default_value); // Serialized graph

	daggle_instance_h daggle;
	daggle_node_get_daggle(handle, &daggle);

	graph_invoker_context_t* ctx = malloc(sizeof *ctx);

	ctx->graph = NULL;
	ctx->handle = handle;
	ctx->daggle = daggle;

	daggle_port_h graph_port;
	daggle_node_get_port_by_name(handle, "graph", &graph_port);
	daggle_port_get_value(graph_port, &ctx->graph);

	if (ctx->graph) {
		graph_invoker_declare_graph_bridges(handle, ctx->graph);
		daggle_node_declare_task(handle, graph_invoker_impl);
	}

	daggle_node_declare_context(handle, ctx, graph_invoker_context_dispose);
}

PLUGIN_API void
initialize(daggle_instance_h instance)
{
	daggle_plugin_register_node(instance, "input_bridge", input_bridge);
	daggle_plugin_register_node(instance, "output_bridge", output_bridge);
	daggle_plugin_register_node(instance, "graph_invoker", graph_invoker);

	daggle_plugin_register_type(instance, "graph_object", clone_graph_object,
		free_graph_object, serialize_graph_object, deserialize_graph_object);
}
