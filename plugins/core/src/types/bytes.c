#include "types/bytes.h"

#include "memory.h"
#include "stdio.h"

void
clone_bytes(daggle_instance_h instance, const void* data, void** target)
{
	uint64_t len = *((uint64_t*)data);

	void* res = malloc(sizeof(uint64_t) + len);
	memcpy(res, &len, sizeof(uint64_t));
	memcpy(res + sizeof(uint64_t), data, len);

	*target = res;
}

void
free_bytes(daggle_instance_h instance, void* data)
{
	free(data);
}

void
serialize_bytes(daggle_instance_h instance, const void* data,
	unsigned char** out_buf, uint64_t* out_len)
{
	uint64_t len = *((uint64_t*)data);
	*out_len = len;
	unsigned char* bytes = malloc(len);
	memcpy(bytes, data + sizeof(uint64_t), len);

	*out_buf = bytes;
}

void
deserialize_bytes(daggle_instance_h instance, const unsigned char* bin,
	uint64_t len, void** target)
{
	if (len <= sizeof(uint64_t)) {
		return;
	}

	void* res = malloc(sizeof(uint64_t) + len);
	memcpy(res, &len, sizeof(uint64_t));
	memcpy(res + sizeof(uint64_t), bin, len);

	*target = res;
}
