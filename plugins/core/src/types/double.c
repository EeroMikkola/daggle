#include "types/double.h"

void
clone_double(
	daggle_instance_h instance, const void* data, void** target)
{
	double* res = malloc(sizeof(double));
	*res = *((const double*)data);
	*target = res;
}

void
free_double(
	daggle_instance_h instance, void* data)
{
	free(data);
}

void
serialize_double(
	daggle_instance_h instance,
	const void* data,
	unsigned char** out_buf,
	uint64_t* out_len)
{
	double val = *(double*)data;
	unsigned char* buf = malloc(sizeof val);
	*buf = val;

	*out_buf = buf;
	*out_len = sizeof val;
}

void
deserialize_double(
	daggle_instance_h instance,
	const unsigned char* bin,
	uint64_t len,
	void** target)
{
	if(len != sizeof(double)) {
		return;
	}

	clone_double(instance, bin, target);
}
