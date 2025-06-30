#pragma once

#include "instance.h"
#include "node.h"
#include "utility/dynamic_array.h"

#include <daggle/daggle.h>

typedef struct graph_s {
	dynamic_array_t nodes;
	instance_t* instance;
	node_t* owner;
	bool locked;
} graph_t;