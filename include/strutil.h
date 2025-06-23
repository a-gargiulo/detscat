#ifndef STRUTIL
#define STRUTIL

char *strutil_trim(char *str);

#ifdef STRUTIL_NO_PREFIX
#define trim strutil_trim
#endif

#endif  // STRUTIL
