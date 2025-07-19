#pragma once

#include "instance.h"
#include "utility/dynamic_array.h"

#include <daggle/daggle.h>

typedef struct graph_s {
	dynamic_array_t nodes;
	instance_t* instance;
	bool locked;
} graph_t;