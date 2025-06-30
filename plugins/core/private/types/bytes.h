#pragma once
#include "stdint.h"
#include "stdlib.h"

#include <daggle/daggle.h>

// Bytes stores a byte array of set length.
// The first 8 bytes denote how many bytes the array is.
// The size of the structure is therefore 8+len bytes.
// struct bytes_s {
//    uint64_t len;
//    unsigned char bytes[len];
//}

void
clone_bytes(daggle_instance_h instance, const void* data, void** target);

void
free_bytes(daggle_instance_h instance, void* data);

void
serialize_bytes(daggle_instance_h instance,
	const void* data,
	unsigned char** out_buf,
	uint64_t* out_len);

void
deserialize_bytes(daggle_instance_h instance,
	const unsigned char* bin,
	uint64_t len,
	void** target);
