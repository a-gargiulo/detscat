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
