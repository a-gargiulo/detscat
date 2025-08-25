#include "str.h"

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define STR_INIT_CAP 16

char *str_raw_trim(char *str) {
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

int str_raw_strcasecmp(const char *s1, const char *s2) {
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

char *str_raw_normpath(char *path) {
    if (path == NULL) return NULL;

    char *p = path;
    while (*p) {
        if (*p == '\\') *p = '/';
        p++;
    }

    return path;
}

char *str_raw_strdup(const char *str) {
    if (!str) return NULL;

    size_t n_str = strlen(str);
    char *buf = malloc(n_str + 1);
    if (!buf) return NULL;

    memcpy(buf, str, n_str + 1);
    return buf;
}

bool str_init(Str *s) {
    if (!s) return false;
    s->data = malloc(STR_INIT_CAP);
    if (!s->data) return false;
    s->data[0] = '\0';
    s->length = 0;
    s->capacity = STR_INIT_CAP;
    return true;
}

void str_free(Str *s) {
    if (!s) return;
    free(s->data);
    s->data = NULL;
    s->length = 0;
    s->capacity = 0;
}

static bool str_grow(Str *s, size_t min_capacity) {
    if (s->capacity >= min_capacity) return true;
    size_t new_cap = s->capacity;
    while (new_cap < min_capacity) {
        new_cap *= 2; // exponential growth
    }
    char *new_data = realloc(s->data, new_cap);
    if (!new_data) return false;
    s->data = new_data;
    s->capacity = new_cap;
    return true;
}

bool str_reserve(Str *s, size_t needed_capacity) {
    return str_grow(s, needed_capacity);
}

bool str_set(Str *s, const char *src) {
    if (!s || !src) return false;
    size_t n = strlen(src);
    if (!str_grow(s, n + 1)) return false;
    memcpy(s->data, src, n + 1);
    s->length = n;
    return true;
}

bool str_copy(Str *dst, const Str *src) {
    if (!dst || !src) return false;
    if (!str_grow(dst, src->length + 1)) return false;
    memcpy(dst->data, src->data, src->length + 1);
    dst->length = src->length;
    return true;
}

bool str_append(Str *s, const char *suffix) {
    if (!s || !suffix) return false;
    size_t slen = strlen(suffix);
    if (!str_grow(s, s->length + slen + 1)) return false;
    memcpy(s->data + s->length, suffix, slen + 1);
    s->length += slen;
    return true;
}

bool str_append_char(Str *s, char c) {
    if (!s) return false;
    if (!str_grow(s, s->length + 2)) return false;
    s->data[s->length++] = c;
    s->data[s->length] = '\0';
    return true;
}
