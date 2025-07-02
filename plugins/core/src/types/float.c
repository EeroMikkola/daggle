#include "types/float.h"

void
clone_float(daggle_instance_h instance, const void* data, void** target)
{
	float* res = malloc(sizeof(float));
	*res = *((const float*)data);
	*target = res;
}

void
free_float(daggle_instance_h instance, void* data)
{
	free(data);
}

void
serialize_float(daggle_instance_h instance, const void* data,
	unsigned char** out_buf, uint64_t* out_len)
{
	float val = *(float*)data;
	unsigned char* buf = malloc(sizeof val);
	*buf = val;

	*out_buf = buf;
	*out_len = sizeof val;
}

void
deserialize_float(daggle_instance_h instance, const unsigned char* bin,
	uint64_t len, void** target)
{
	if (len != sizeof(float)) {
		return;
	}

	clone_float(instance, bin, target);
}
