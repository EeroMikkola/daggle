#pragma once

#include "stdint.h"

#include <daggle/daggle.h>

typedef struct dynamic_array_s {
	uint64_t capacity;
	uint64_t length;
	uint64_t stride;
	void* data;
} dynamic_array_t;

// Guaranteed to return DAGGLE_SUCCESS, if capacity == 0
daggle_error_code_t
dynamic_array_init(uint64_t capacity, uint64_t stride, dynamic_array_t* array);

void
dynamic_array_destroy(dynamic_array_t* array);

// Copies stride bytes (or bits, not sure) from data.
// If data is NULL, the element will NOT be initialized.
// To set the data to NULL, data should point to NULL pointer.
// Guaranteed to return DAGGLE_SUCCESS, if length < capacity
daggle_error_code_t
dynamic_array_push(dynamic_array_t* array, void* data);

void
dynamic_array_remove(dynamic_array_t* array, uint64_t index);

void*
dynamic_array_at(const dynamic_array_t* array, uint64_t index);

// Take the ownership of the data. This will set data to null, capacity and
// length to 0. Stride remains.
void
dynamic_array_steal(dynamic_array_t* array);

void
dynamic_array_resize(dynamic_array_t* array, uint64_t new_capacity);