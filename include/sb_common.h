#ifndef SB_COMMON_H
#define SB_COMMON_H

#include <assert.h>
#include <memory.h>
#include <stdio.h>

#define COUNTOF(arr) sizeof((arr)) / sizeof(*(arr))

#define SB_PANIC(message) \
	do { \
		fprintf(stderr, "Error at line %d in file %s: %s\n",__LINE__, __FILE__, message); \
		exit(1); \
	} while (0)

#define VK_CHECK(call) \
	do { \
		VkResult result_ = call; \
		assert(result_ == VK_SUCCESS); \
	} while (0)

#define SB_ZERO(ptr, size) memset((ptr), 0, (size))
#define SB_ZERO_ARRAY(arr) SB_ZERO(arr, sizeof((arr)))
#define SB_ZERO_STRUCT(ptr) SB_ZERO(ptr, sizeof(*(ptr)))

#endif
