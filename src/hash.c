#include "utility/hash.h"

uint32_t
fnv1a_32(
	const char* str)
{
	uint32_t hash = 0x811c9dc5;

	unsigned char* s = (unsigned char*)str;
	while(*s) {
		hash ^= *s++;
		hash *= 0x01000193;
	}

	return hash;
}

uint64_t
fnv1a_64(const char* str)
{
	uint64_t hash = 0xcbf29ce484222325;

	unsigned char* s = (unsigned char*)str;
	while(*s) {
		hash ^= *s++;
		hash *= 0x00000100000001b3;
	}

	return hash;
}