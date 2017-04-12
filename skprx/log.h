#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <string.h>

#define LOG_PATH "ux0:dump/"
#define LOG_FILE LOG_PATH "vitastick.txt"

void log_reset();
void log_write(const char *buffer, size_t length);
void log_flush();

#ifndef RELEASE
#  define LOG(...) \
	do { \
		char buffer[256]; \
		char time_buf[32]; \
		snprintf(time_buf, sizeof(time_buf), "[%lld.%lld] ", ksceKernelGetSystemTimeWide() / 1000000, (ksceKernelGetSystemTimeWide() % 1000000) / 1000); \
		snprintf(buffer, sizeof(buffer), ##__VA_ARGS__); \
		log_write(time_buf, strlen(time_buf)); \
		log_write(buffer, strlen(buffer)); \
	} while (0)
#else
#  define LOG(...) (void)0
#endif

#define TEST_CALL(f, ...) ({ \
	int ret = f(__VA_ARGS__); \
	LOG(# f " returned 0x%08X\n", ret); \
	ret; \
})

#endif
