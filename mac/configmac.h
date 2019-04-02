// Macintosh-specific include file

#include "../src/configunix.h"
#define USE_AQUA_THEME 1

#if __clang_major__ >= 11
#pragma clang diagnostic ignored "-Wunderaligned-exception-object"
#endif

#define MAC_OS_X_VERSION_MAX_ALLOWED 1070
#define MAC_OS_X_VERSION_MIN_REQUIRED 1070
