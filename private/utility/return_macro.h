#pragma once
#include "log_macro.h"
#include "stdio.h"
#include "string.h"

#include <daggle/daggle.h>

static const char* daggle_error_code_strings[] = {
	"DAGGLE_SUCCESS",
	"DAGGLE_ERROR_UNKNOWN",
	"DAGGLE_ERROR_NULL_PARAMETER",
	"DAGGLE_ERROR_NULL_OUTPUT_PARAMETER",
	"DAGGLE_ERROR_PARSE",
	"DAGGLE_ERROR_MEMORY_ALLOCATION",
	"DAGGLE_ERROR_INCORRECT_PORT_VARIANT",
};

#ifdef DAGGLE_ENABLE_RETURN_STATUS_ERROR_LOGS
#define RETURN_STATUS_DAGGLE_ENABLE_RETURN_STATUS_ERROR_LOGS_BEHAVIOR          \
	do {                                                                       \
		LOG(LOG_TAG_ERROR, daggle_error_code_strings[_daggle_error_rs]);       \
	} while(0)
#else
#define RETURN_STATUS_DAGGLE_ENABLE_RETURN_STATUS_ERROR_LOGS_BEHAVIOR          \
	do {                                                                       \
	} while(0)
#endif

#ifdef DAGGLE_ENABLE_RETURN_STATUS_SUCCESS_LOGS
#define RETURN_STATUS_DAGGLE_ENABLE_RETURN_STATUS_SUCCESS_LOGS_BEHAVIOR        \
	do {                                                                       \
		LOG(LOG_TAG_DEBUG, "DAGGLE_SUCCESS");                                  \
	} while(0)
#else
#define RETURN_STATUS_DAGGLE_ENABLE_RETURN_STATUS_SUCCESS_LOGS_BEHAVIOR        \
	do {                                                                       \
	} while(0)
#endif

#define RETURN_STATUS(expression)                                              \
	do {                                                                       \
		daggle_error_code_t _daggle_error_rs = expression;                     \
		if(_daggle_error_rs != DAGGLE_SUCCESS) {                               \
			RETURN_STATUS_DAGGLE_ENABLE_RETURN_STATUS_ERROR_LOGS_BEHAVIOR;     \
		} else {                                                               \
			RETURN_STATUS_DAGGLE_ENABLE_RETURN_STATUS_SUCCESS_LOGS_BEHAVIOR;   \
		}                                                                      \
		return _daggle_error_rs;                                               \
	} while(0)

// Return the status code of the expression if it's not DAGGLE_SUCCESS.
#define RETURN_IF_ERROR(expression)                                            \
	do {                                                                       \
		daggle_error_code_t _daggle_error_rie = expression;                    \
		if(_daggle_error_rie != DAGGLE_SUCCESS) {                              \
			RETURN_STATUS(_daggle_error_rie);                                  \
		}                                                                      \
	} while(0)

#define RETURN_X_DAGGLE_ERROR_IF_NULL(error, expression)                       \
	do {                                                                       \
		if(expression == NULL) {                                               \
			RETURN_STATUS(error);                                              \
		}                                                                      \
	} while(0)

// Return DAGGLE_ERROR_NULL_OUTPUT_PARAMETER if expression equals NULL.
#define REQUIRE_OUTPUT_PARAMETER(expression)                                   \
	RETURN_X_DAGGLE_ERROR_IF_NULL(                                             \
		DAGGLE_ERROR_NULL_OUTPUT_PARAMETER, expression)

// Return DAGGLE_ERROR_NULL_PARAMETER if expression equals NULL.
#define REQUIRE_PARAMETER(expression)                                          \
	RETURN_X_DAGGLE_ERROR_IF_NULL(DAGGLE_ERROR_NULL_PARAMETER, expression)

// Return DAGGLE_ERROR_MEMORY_ALLOCATION if expression equals NULL.
#define REQUIRE_ALLOCATION_DAGGLE_SUCCESSFUL(expression)                       \
	RETURN_X_DAGGLE_ERROR_IF_NULL(DAGGLE_ERROR_MEMORY_ALLOCATION, expression)

#ifdef DAGGLE_ENABLE_ASSERT
#define ASSERT_TRUE(expression, message)                                       \
	do {                                                                       \
		if(!(expression)) {                                                    \
			LOG(LOG_TAG_ERROR, message);                                       \
		}                                                                      \
	} while(0)

#define ASSERT_NOT_NULL(expression, message)                                   \
	do {                                                                       \
		ASSERT_TRUE((expression != NULL), message);                            \
	} while(0)

#define ASSERT_SUCCESS(expression)                                             \
	do {                                                                       \
		daggle_error_code_t _daggle_error_as = expression;                     \
		ASSERT_TRUE(_daggle_error_as == DAGGLE_SUCCESS,                        \
			daggle_error_code_strings[_daggle_error_as]);                      \
	} while(0)

#define ASSERT_OUTPUT_PARAMETER(expression)                                    \
	do {                                                                       \
		ASSERT_NOT_NULL(expression, "Output parameter is null");               \
	} while(0)

#define ASSERT_PARAMETER(expression)                                           \
	do {                                                                       \
		ASSERT_NOT_NULL(expression, "Parameter is null");                      \
	} while(0)

#else
#define ASSERT_TRUE(expression, message)                                       \
	do {                                                                       \
	} while(0)
#define ASSERT_NOT_NULL(expression, message)                                   \
	do {                                                                       \
	} while(0)
#define ASSERT_SUCCESS(expression)                                             \
	do {                                                                       \
	} while(0)
#define ASSERT_OUTPUT_PARAMETER(expression)                                    \
	do {                                                                       \
	} while(0)
#define ASSERT_PARAMETER(expression)                                           \
	do {                                                                       \
	} while(0)
#endif

#define GOTO_IF_ERROR(expression, label)                                       \
	do {                                                                       \
		if(expression != DAGGLE_SUCCESS) {                                     \
			goto label;                                                        \
		}                                                                      \
	} while(0)
