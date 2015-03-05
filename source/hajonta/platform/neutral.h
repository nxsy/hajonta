//
// This file is for defines or functions that create platform neutrality
// without requiring the platform headers to be loaded either.
//
// For example, this is useful for bootstrap programs, unit tests, and so forth
// that aren't actually loading the platform layer or game code.

#if defined(_WIN32)
#include <windows.h>
#define SLASH "\\"
#elif defined(__linux__)
#include <linux/limits.h>
#define MAX_PATH PATH_MAX
#define _snprintf snprintf
#define SLASH "/"
#elif defined(__APPLE__)
#if defined(__MACH__)
#include <sys/syslimits.h>
#define MAX_PATH PATH_MAX
#define _snprintf snprintf
#define SLASH "/"
#endif
#endif
