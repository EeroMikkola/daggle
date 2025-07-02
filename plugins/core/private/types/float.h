#pragma once
#include "stdint.h"
#include "stdlib.h"

#include <daggle/daggle.h>

void
clone_float(daggle_instance_h instance, const void* data, void** target);

void
free_float(daggle_instance_h instance, void* data);

void
serialize_float(daggle_instance_h instance, const void* data,
	unsigned char** out_buf, uint64_t* out_len);

void
deserialize_float(daggle_instance_h instance, const unsigned char* bin,
	uint64_t len, void** target);
