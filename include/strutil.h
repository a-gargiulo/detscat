#ifndef STRUTIL
#define STRUTIL

char *strutil_trim(char *str);
int strutil_strcasecmp(const char *s1, const char *s2);

#ifdef STRUTIL_NO_PREFIX
#define trim strutil_trim
#endif

#endif  // STRUTIL
