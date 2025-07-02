#pragma once
#include "stdio.h"
#include "string.h"

#define ANSI_EC_RED "\033[31m"
#define ANSI_EC_GREEN "\033[32m"
#define ANSI_EC_BLUE "\033[34m"
#define ANSI_EC_YELLOW "\033[33m"
#define ANSI_EC_DIM "\033[2m"
#define ANSI_EC_RESET "\033[0m"

#define LOG_TAG_DEBUG ANSI_EC_GREEN "[DEBUG]" ANSI_EC_RESET
#define LOG_TAG_INFO ANSI_EC_BLUE "[INFO]" ANSI_EC_RESET
#define LOG_TAG_WARN ANSI_EC_YELLOW "[WARN]" ANSI_EC_RESET
#define LOG_TAG_ERROR ANSI_EC_RED "[ERROR]" ANSI_EC_RESET

#ifdef DAGGLE_ENABLE_LOG
#ifdef DAGGLE_ENABLE_LOG_LOCATION
// https://stackoverflow.com/questions/8487986/file-macro-shows-full-path
#define __FILENAME__                                                           \
	(strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define LOG_FMT(tag, format, ...)                                              \
	do {                                                                       \
		printf(tag " " format ANSI_EC_RESET " " ANSI_EC_DIM                    \
				   "(%s() in %s:%i)" ANSI_EC_RESET "\n",                       \
			__VA_ARGS__, __FUNCTION__, __FILENAME__, __LINE__);                \
	} while (0)
#else
#define LOG_FMT(tag, format, ...)                                              \
	do {                                                                       \
		printf(tag " " format ANSI_EC_RESET "\n", __VA_ARGS__);                \
	} while (0)
#endif
#else
#define LOG_FMT(tag, format, ...)                                              \
	do {                                                                       \
	} while (0)
#endif

#define LOG(tag, message)                                                      \
	do {                                                                       \
		LOG_FMT(tag, "%s", message);                                           \
	} while (0)

#ifdef DAGGLE_ENABLE_LOG_CONDITIONAL_DEBUG
#define LOG_FMT_COND_DEBUG(format, ...)                                        \
	do {                                                                       \
		LOG_FMT(LOG_TAG_DEBUG, format, __VA_ARGS__);                           \
	} while (0)
#else
#define LOG_FMT_COND_DEBUG(format, ...)                                        \
	do {                                                                       \
	} while (0)
#endif

#define LOG_COND_DEBUG(message)                                                \
	do {                                                                       \
		LOG_FMT_COND_DEBUG("%s", message);                                     \
	} while (0)
