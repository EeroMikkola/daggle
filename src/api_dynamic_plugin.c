#include "plugin_manager.h"
#include "stdlib.h"
#include "string.h"
#include "stdbool.h"
#include "utility/return_macro.h"

#include <daggle/daggle.h>

#ifdef _WIN32
#include "windows.h"
#define HANDLE_TYPE HMODULE
#define LOAD_LIBRARY(path) LoadLibrary(path)
#define GET_PROC_ADDRESS(handle, func) GetProcAddress(handle, func)
#define FREE_LIBRARY(handle) FreeLibrary(handle)
#define PLATFORM_CODE "win"
#endif
#ifdef __APPLE__
#include "dlfcn.h"
#define HANDLE_TYPE void*
#define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY)
#define GET_PROC_ADDRESS(handle, func) dlsym(handle, func)
#define FREE_LIBRARY(handle) dlclose(handle)
#define PLATFORM_CODE "osx"
#endif
#ifdef __linux__
#include "dlfcn.h"
#define HANDLE_TYPE void*
#define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY)
#define GET_PROC_ADDRESS(handle, func) dlsym(handle, func)
#define FREE_LIBRARY(handle) dlclose(handle)
#define PLATFORM_CODE "linux"
#endif

typedef void (*prv_dp_init)(daggle_instance_h instance);

typedef struct prv_dp_interface_context_s {
	HANDLE_TYPE handle;
} prv_dp_interface_context_t;

typedef struct prv_dp_source_impl_context_s {
	char* binary_path;
} prv_dp_source_impl_context_t;

void
prv_dp_interface_init(daggle_instance_h instance, void* context)
{
	ASSERT_PARAMETER(instance);
	ASSERT_PARAMETER(context);

	prv_dp_interface_context_t* ctx = context;
	HANDLE_TYPE handle = ctx->handle;

	ASSERT_NOT_NULL(handle, "Dynamic plugin handle is null");

	prv_dp_init init
		= (prv_dp_init)(void*)GET_PROC_ADDRESS(handle, "initialize");

	if (!init) {
		LOG(LOG_TAG_ERROR, "Plugin entrypoint not found");
		return;
	}

	init(instance);
}

void
prv_dp_interface_shutdown(daggle_instance_h instance, void* context)
{
	ASSERT_PARAMETER(instance);
	ASSERT_PARAMETER(context);

	prv_dp_interface_context_t* ctx = context;

	ASSERT_NOT_NULL(ctx->handle, "Dynamic plugin handle is null");

	FREE_LIBRARY(ctx->handle);
	free(ctx);
}

void
prv_dp_source_impl_context_load(struct daggle_plugin_source_s* source,
	daggle_plugin_interface_t* out_interface)
{
	ASSERT_PARAMETER(source);
	ASSERT_PARAMETER(out_interface);

	prv_dp_source_impl_context_t* context = source->context;

	out_interface->init = prv_dp_interface_init;
	out_interface->shutdown = prv_dp_interface_shutdown;

	prv_dp_interface_context_t* ctx = malloc(sizeof *ctx);

	ctx->handle = LOAD_LIBRARY(context->binary_path);

	out_interface->context = ctx;
}

void
prv_dp_source_impl_context_free(struct daggle_plugin_source_s* source)
{
	ASSERT_PARAMETER(source);

	prv_dp_source_impl_context_t* context = source->context;

	free(context->binary_path);
	free(context);
}

void
prv_duplicate_string(char* value, char** out_target)
{
	uint32_t id_len = strlen(value);
	*out_target = malloc(sizeof(char) * id_len + 1);
	memcpy(*out_target, value, id_len);
	(*out_target)[id_len] = '\0';
}

daggle_error_code_t
prv_parse_file_ini_next(FILE* file, char* section_buffer, char* key_buffer,
	char* value_buffer, bool* out_stop)
{
	char line[256];
	while (fgets(line, sizeof(line), file)) {
		// Skip empty lines
		if (line[0] == '\n') {
			continue;
		}

		// Skip comments
		if (line[0] == '#') {
			continue;
		}

		// Get the length of the current line
		uint32_t line_len = strlen(line);

		// If the line is a section, copy the section name into section buffer.
		if (line[0] == '[') {
			memcpy(section_buffer, line + 1, line_len - 3);
			section_buffer[line_len - 3] = '\0';
			continue;
		}

		char* delimiter = strchr(line, '=');

		if (!delimiter) {
			LOG(LOG_TAG_ERROR, "Failed to find a delimiter");
			*out_stop = true;
			RETURN_STATUS(DAGGLE_ERROR_PARSE);
		}

		// TODO: instead of this: "k=v", support this: "  k = v "
		uint32_t key_len = delimiter - line;
		memcpy(key_buffer, line, key_len);
		key_buffer[key_len] = '\0';

		// If the line ends with newline, ignore the last character
		uint32_t value_len = line_len - key_len - 1;
		if (delimiter[value_len] == '\n') {
			value_len -= 1;
		}

		memcpy(value_buffer, delimiter + 1, value_len);
		value_buffer[value_len] = '\0';

		*out_stop = false;
		RETURN_STATUS(DAGGLE_SUCCESS);
	}

	*out_stop = true;
	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
prv_dp_parse_plugin(const char* path,
	daggle_plugin_source_t* out_source, char** out_binary_path)
{
	ASSERT_PARAMETER(path);
	ASSERT_PARAMETER(out_source);
	ASSERT_PARAMETER(out_binary_path);

	FILE* file = fopen(path, "r");

	if (!file) {
		LOG_FMT(LOG_TAG_ERROR, "Plugin at path %s not found\n", path);
		RETURN_STATUS(DAGGLE_ERROR_UNKNOWN);
	}

	// Initialize source with default values.
	out_source->id = NULL;
	out_source->dependencies = NULL;
	out_source->abi = UINT32_MAX;

	char section[256];
	char key[256];
	char value[256];

	// Set strings to null.
	memset(section, 0, 256);
	memset(key, 0, 256);
	memset(value, 0, 256);

	bool should_stop = false;
	do {
		daggle_error_code_t error = prv_parse_file_ini_next(file, section, key, value, &should_stop);
		if(error) {
			RETURN_STATUS(error);
		}

		if(should_stop) {
			break;
		}

		if (strcmp(section, "plugin") == 0) {
			if (strcmp(key, "id") == 0) {
				prv_duplicate_string(value,
					&out_source->id);
			}
			if (strcmp(key, "abi") == 0) {
				uint32_t abi = strtoul(value, NULL, 10);
				out_source->abi = abi;
			}
			if (strcmp(key, "dependencies") == 0) {
				prv_duplicate_string(value, &out_source->dependencies);
			}
		} else if (strcmp(section, "bin") == 0) {
			if (strcmp(key, PLATFORM_CODE) == 0) {
				char* bin_path;
				prv_duplicate_string(value, &bin_path);

				// TODO: Validate binary path

				*out_binary_path = bin_path;
			}
		}
	} while(!should_stop);

	fclose(file);

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
prv_dynamic_plugin_create_source(const char* path,
	daggle_plugin_source_t* out_descriptor)
{
	ASSERT_PARAMETER(path);
	ASSERT_OUTPUT_PARAMETER(out_descriptor);

	daggle_plugin_source_t plugin_descriptor;
	char* binary_path = NULL;

	// Load plugin id, versions, dependencies, binary path
	daggle_error_code_t error
		= prv_dp_parse_plugin(path, &plugin_descriptor, &binary_path);

	if (error != DAGGLE_SUCCESS) {
		LOG(LOG_TAG_ERROR, "Plugin parsing failed");
		RETURN_STATUS(error);
	}

	// If the plugin file did not define plugin binary path
	if (!binary_path) {
		LOG(LOG_TAG_ERROR, "Plugin binary for " PLATFORM_CODE " is missing");
		RETURN_STATUS(DAGGLE_ERROR_PARSE);
	}

	// TODO: validate binary_path

	// Create a plugin context to hold the plugin library handle
	prv_dp_source_impl_context_t* ctx = malloc(sizeof *ctx);
	REQUIRE_ALLOCATION_DAGGLE_SUCCESSFUL(ctx);

	ctx->binary_path = binary_path;

	// Set the descriptor context
	plugin_descriptor.context = ctx;

	// Set descriptor functions
	plugin_descriptor.load = prv_dp_source_impl_context_load;
	plugin_descriptor.dispose = prv_dp_source_impl_context_free;

	// Copy the plugin definition back to the caller.
	//*out_descriptor = plugin_descriptor;
	memcpy(out_descriptor, &plugin_descriptor, sizeof(daggle_plugin_source_t));

	RETURN_STATUS(DAGGLE_SUCCESS);
}

daggle_error_code_t
daggle_plugin_source_create_from_file(const char* path,
	daggle_plugin_source_t* out_plugin_source)
{
	REQUIRE_PARAMETER(path);
	REQUIRE_OUTPUT_PARAMETER(out_plugin_source);

	daggle_plugin_source_t source;
	RETURN_IF_ERROR(prv_dynamic_plugin_create_source(path, &source));

	*out_plugin_source = source;

	RETURN_STATUS(DAGGLE_SUCCESS);
}
