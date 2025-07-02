#include "stdlib.h"
#include "utility/return_macro.h"

#include <daggle/daggle.h>

const char*
get_version(void)
{
	return DAGGLE_VERSION_STRING;
}

uint32_t
get_abi(void)
{
	return DAGGLE_ABI_VERSION;
}