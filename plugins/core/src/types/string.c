#include "types/string.h"

#include "stdlib.h"
#include "string.h"

void
clone_string(daggle_instance_h instance, const void* data, void** target)
{
	uint64_t len = strlen((const char*)data);

	char* res = malloc(len);
	memcpy(res, (const char*)data, sizeof(char) * len);
	((char*)target)[len] = '\0';

	*target = res;
}

void
free_string(daggle_instance_h instance, void* data)
{
	free(data);
}

void
serialize_string(daggle_instance_h instance, const void* data,
	unsigned char** out_buf, uint64_t* out_len)
{
	uint64_t len = strlen((char*)data);
	unsigned char* buf = malloc(sizeof(unsigned char) * len);
	memcpy(buf, data, len);

	*out_buf = buf;
	*out_len = len;
}

void
deserialize_string(daggle_instance_h instance, const unsigned char* bin,
	uint64_t len, void** target)
{
	if (len < sizeof(char)) {
		return;
	}

	char* res = malloc(len);
	memcpy(res, (const char*)bin, sizeof(char) * len);
	res[len] = '\0';

	*target = res;
}
