#pragma once

#define DEFAULT_VALUE_GENERATOR(fname, type, val, key)                         \
	bool fname(void** out_data, const char** out_type)                         \
	{                                                                          \
		type* data = malloc(sizeof *data);                                     \
		*data = val;                                                           \
		*out_data = data;                                                      \
		*out_type = key;                                                       \
		return true;                                                           \
	}
