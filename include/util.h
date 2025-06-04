#ifndef UTIL
#define UTIL

char* util_trim(char* str);

#ifdef UTIL_USE_NO_PREFIX
#define trim util_trim
#endif

#endif // UTIL
