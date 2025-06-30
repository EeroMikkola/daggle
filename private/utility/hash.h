#pragma once

#include "stdint.h"

uint32_t
fnv1a_32(const char* str);

uint64_t
fnv1a_64(const char* str);