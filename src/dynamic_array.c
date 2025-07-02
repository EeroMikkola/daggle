#include "utility/dynamic_array.h"

#include "stdlib.h"
#include "string.h"
#include "utility/return_macro.h"

daggle_error_code_t
dynamic_array_init(uint64_t capacity, uint64_t stride, dynamic_array_t* array)
{
	ASSERT_PARAMETER(array);
	ASSERT_TRUE(stride > 0, "Stride must be larger than 0");

	array->capacity = capacity;
	array->length = 0;
	array->stride = stride;

	if (capacity > 0) {
		array->data = malloc(stride * capacity);
		REQUIRE_ALLOCATION_DAGGLE_SUCCESSFUL(array->data);
	} else {
		array->data = NULL;
	}

	RETURN_STATUS(DAGGLE_SUCCESS);
}

void
dynamic_array_destroy(dynamic_array_t* array)
{
	ASSERT_PARAMETER(array);

	// Data is NULL when capacity is 0.
	// Free the data only if it exists.
	if (array->data) {
		free(array->data);
	}

	array->data = NULL;
	array->capacity = 0;
	array->length = 0;
	array->stride = 0;
}

daggle_error_code_t
dynamic_array_push(dynamic_array_t* array, void* data)
{
	ASSERT_PARAMETER(array);

	if (array->length == array->capacity) {
		uint64_t new_capacity
			= (array->capacity == 0) ? 1 : array->capacity * 2;
		void* new_array = realloc(array->data, new_capacity * array->stride);
		REQUIRE_ALLOCATION_DAGGLE_SUCCESSFUL(new_array);

		array->data = new_array;
		array->capacity = new_capacity;
	}

	unsigned char* target
		= (unsigned char*)array->data + (array->length * array->stride);

	if (data) {
		// If data is provided, copy stride-bytes from the data.
		memcpy(target, data, array->stride);
	} else {
		// If one wants to write NULL, the data should be a pointer a memory
		// location with the data being NULL.
		memset(target, '\0', sizeof(char) * array->stride);
	}

	array->length++;
	RETURN_STATUS(DAGGLE_SUCCESS);
}

void
dynamic_array_remove(dynamic_array_t* array, uint64_t index)
{
	ASSERT_PARAMETER(array);
	ASSERT_TRUE(index < array->length, "index out of bounds");

	if (index < array->length - 1) {
		unsigned char* dest
			= (unsigned char*)array->data + (index * array->stride);
		unsigned char* src
			= (unsigned char*)array->data + ((index + 1) * array->stride);
		uint64_t bytes_to_move = (array->length - index - 1) * array->stride;
		memcpy(dest, src, bytes_to_move);
	}

	array->length--;
}

void*
dynamic_array_at(const dynamic_array_t* array, uint64_t index)
{
	ASSERT_PARAMETER(array);
	ASSERT_TRUE(index < array->length, "index out of bounds");

	return (unsigned char*)array->data + (index * array->stride);
}

void
dynamic_array_steal(dynamic_array_t* array)
{
	ASSERT_PARAMETER(array);

	array->data = NULL;
	array->capacity = 0;
	array->length = 0;
}

void
dynamic_array_resize(dynamic_array_t* array, uint64_t new_capacity)
{
	ASSERT_PARAMETER(array);

	ASSERT_TRUE(new_capacity >= array->length,
		"New capacity may not be less than current length");

	void* new_location = realloc(array->data, new_capacity * array->stride);
	if (new_location) {
		array->data = new_location;
		array->capacity = new_capacity;
	}
}