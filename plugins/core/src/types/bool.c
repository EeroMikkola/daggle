#include "types/bool.h"

#include "stdbool.h"

void
clone_bool(
	daggle_instance_h instance, const void* data, void** target)
{
	bool* res = malloc(sizeof(bool));
	*res = *((const bool*)data);
	*target = res;
}

void
free_bool(
	daggle_instance_h instance, void* data)
{
	free(data);
}

void
serialize_bool(
	daggle_instance_h instance,
	const void* data,
	unsigned char** out_buf,
	uint64_t* out_len)
{
	bool val = *(bool*)data;
	unsigned char* buf = malloc(sizeof val);
	*buf = val;

	*out_buf = buf;
	*out_len = sizeof val;
}

void
deserialize_bool(
	daggle_instance_h instance,
	const unsigned char* bin,
	uint64_t len,
	void** target)
{
	if(len != sizeof(bool)) {
		return;
	}

	clone_bool(instance, bin, target);
}
