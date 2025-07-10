#include "strutil.h"

#include <ctype.h>
#include <stddef.h>

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
    while (*s1 && *s2) {
        c1 = (unsigned char)tolower((unsigned char)*s1);
        c2 = (unsigned char)tolower((unsigned char)*s2);
        if (c1 != c2)
            return c1 - c2;
        s1++;
        s2++;
    }
    return (unsigned char)tolower((unsigned char)*s1) -
           (unsigned char)tolower((unsigned char)*s2);
}
