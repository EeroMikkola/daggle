#pragma once

#include "executor.h"
#include "plugin_manager.h"

#include <daggle/daggle.h>

typedef struct instance_s {
	plugin_manager_t plugin_manager;
	executor_t executor;
} instance_t;
