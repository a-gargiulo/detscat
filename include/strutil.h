#ifndef STRUTIL_H
#define STRUTIL_H

char *strutil_trim(char *str);                              // Trim leading and trailing white spaces from a string (in place) 

int strutil_strcasecmp(const char *s1, const char *s2);     // Compare two strings (case insensitive)

char *strutil_normpath(char *path);                         // Normalize a path
#endif  // STRUTIL_H
