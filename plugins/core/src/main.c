#include "nodes/input.h"
#include "nodes/math.h"
#include "nodes/output.h"
#include "types.h"
#include "types/bool.h"
#include "types/bytes.h"
#include "types/double.h"
#include "types/float.h"
#include "types/int.h"
#include "types/string.h"

#include <daggle/daggle.h>

#ifdef _WIN32
#define PLUGIN_API __declspec(dllexport)
#else
#define PLUGIN_API __attribute__((visibility("default")))
#endif

PLUGIN_API void
initialize(daggle_instance_h instance)
{
	daggle_plugin_register_type(instance,
		INT_TYPE,
		clone_int,
		free_int,
		serialize_int,
		deserialize_int);

	daggle_plugin_register_type(instance,
		FLOAT_TYPE,
		clone_float,
		free_float,
		serialize_float,
		deserialize_float);

	daggle_plugin_register_type(instance,
		DOUBLE_TYPE,
		clone_double,
		free_float,
		serialize_double,
		deserialize_double);

	daggle_plugin_register_type(instance,
		BOOL_TYPE,
		clone_bool,
		free_bool,
		serialize_bool,
		deserialize_bool);

	daggle_plugin_register_type(instance,
		STRING_TYPE,
		clone_string,
		free_string,
		serialize_string,
		deserialize_string);

	daggle_plugin_register_type(instance,
		BYTES_TYPE,
		clone_bytes,
		free_bytes,
		serialize_bytes,
		deserialize_bytes);

	daggle_plugin_register_node(instance, "input", input);
	daggle_plugin_register_node(instance, "math", math);
	daggle_plugin_register_node(instance, "output", output);
}
