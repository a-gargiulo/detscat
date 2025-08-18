#include "strutil.h"

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

char *strutil_trim(char *str) {
    if (str == NULL) return NULL;

    char *start = str;
    while (isspace((unsigned char)*start)) start++;

    if (*start == '\0') {
        *str = '\0';
        return str;
    }

    // use 'dst' to shift the trimmed string to the start of 'str',
    // preserving 'str' as the original base pointer.
    char *dst;
    if (start != str) {
        dst = str;
        while ((*dst++ = *start++));
    }

    char *end;
    end = str;
    while (*end) end++;
    end--;

    while (end >= str && isspace((unsigned char)*end)) *end-- = '\0';

    return str;
}

int strutil_strcasecmp(const char *s1, const char *s2) {
    unsigned char c1, c2;
    while (*s1 || *s2) {
        c1 = (unsigned char)*s1;
        c2 = (unsigned char)*s2;
        c1 = (unsigned char)tolower(c1);
        c2 = (unsigned char)tolower(c2);
        if (c1 != c2) return c1 - c2;
        if (*s1) s1++;
        if (*s2) s2++;
    }
    return 0;
}

char *strutil_normpath(char *path) {
    if (path == NULL) return NULL;

    char *p = path;
    while (*p) {
        if (*p == '\\') *p = '/';
        p++;
    }

    return path;
}

char *strutil_strdup(const char *str) {
    if (!str) return NULL;

    size_t n_str = strlen(str);
    char *buf = malloc(n_str + 1);
    if (!buf) return NULL;

    memcpy(buf, str, n_str + 1);
    return buf;
}
