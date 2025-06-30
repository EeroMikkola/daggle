#include "types/int.h"

#include "string.h"

void
clone_int(
	daggle_instance_h instance, const void* data, void** target)
{
	int32_t* res = malloc(sizeof(int32_t));
	*res = *((const int32_t*)data);
	*target = res;
}

void
free_int(
	daggle_instance_h instance, void* data)
{
	free(data);
}

void
serialize_int(
	daggle_instance_h instance,
	const void* data,
	unsigned char** out_buf,
	uint64_t* out_len)
{
	int32_t val = *(int32_t*)data;
	unsigned char* buf = malloc(sizeof val);
	memcpy(buf, &val, sizeof val);

	*out_buf = buf;
	*out_len = sizeof val;
}

void
deserialize_int(
	daggle_instance_h instance,
	const unsigned char* bin,
	uint64_t len,
	void** target)
{
	if(len != sizeof(int32_t)) {
		return;
	}

	clone_int(instance, bin, target);
}
