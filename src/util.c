#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "util.h"

char* util_trim(char* str) {
    if (!str) return NULL;

    // Trim leading spaces
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) return str; // All spaces

    // Trim trailing spaces
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end + 1) = '\0';

    return str;
}
