#include "detscat_str.h"

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


char *detscat_str_raw_trim(char *s) {
    if (s == NULL) return NULL;

    char *start = s;
    while (isspace((unsigned char)*start)) start++;

    if (*start == '\0') {
        *s = '\0';
        return s;
    }

    // use 'dst' to shift the trimmed sing to the start of 's',
    // preserving 's' as the original base pointer.
    char *dst = s;
    if (start != s) {
        while ((*dst++ = *start++));
    }

    char *end;
    end = s;
    while (*end) end++;
    end--;

    while (end >= s && isspace((unsigned char)*end)) *end-- = '\0';

    return s;
}

int detscat_str_raw_strcasecmp(const char *s1, const char *s2) {
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

char *detscat_str_raw_normpath(char *path) {
    if (path == NULL) return NULL;

    char *p = path;
    while (*p) {
        if (*p == '\\') *p = '/';
        p++;
    }

    return path;
}

char *detscat_str_raw_strdup(const char *s) {
    if (!s) return NULL;

    size_t n_s = strlen(s);
    char *buf = malloc(n_s + 1);
    if (!buf) return NULL;

    memcpy(buf, s, n_s + 1);
    return buf;
}

bool detscat_str_init(Str *s) {
    if (!s) return false;
    s->data = malloc(STR_INIT_CAP);
    if (!s->data) return false;
    s->data[0] = '\0';
    s->length = 0;
    s->capacity = STR_INIT_CAP;
    return true;
}

void detscat_str_free(Str *s) {
    if (!s) return;
    free(s->data);
    s->data = NULL;
    s->length = 0;
    s->capacity = 0;
}

static bool detscat_str_grow(Str *s, size_t min_capacity) {
    if (!s || min_capacity == 0) return false;
    if (min_capacity > STR_MAX_CAP) return false;

    if (s->capacity >= min_capacity) return true;

    size_t new_cap = s->capacity ? s->capacity : STR_INIT_CAP;
    while (new_cap < min_capacity)  {
        if (new_cap > STR_MAX_CAP / 2) {
            new_cap = STR_MAX_CAP;
            break;
        }
        new_cap *= 2;
    }
    
    char *new_data = realloc(s->data, new_cap);
    if (!new_data) return false;

    s->data = new_data;
    s->capacity = new_cap;

    return true;
}

bool detscat_str_reserve(Str *s, size_t needed_cap) {
    if (!s || needed_cap == 0) return false;
    return detscat_str_grow(s, needed_cap);
}

bool detscat_str_set(Str *s, const char *src) {
    if (!s || !src) return false;
    size_t n = strlen(src);
    if (!detscat_str_grow(s, n + 1)) return false;
    memcpy(s->data, src, n + 1);
    s->length = n;
    return true;
}

bool detscat_str_copy(Str *dst, const Str *src) {
    if (!dst || !src) return false;
    if (!detscat_str_grow(dst, src->length + 1)) return false;
    memcpy(dst->data, src->data, src->length + 1);
    dst->length = src->length;
    return true;
}

bool detscat_str_append(Str *s, const char *suffix) {
    if (!s || !suffix) return false;
    size_t slen = strlen(suffix);
    if (!detscat_str_grow(s, s->length + slen + 1)) return false;
    memcpy(s->data + s->length, suffix, slen + 1);
    s->length += slen;
    return true;
}

bool detscat_str_append_char(Str *s, char c) {
    if (!s) return false;
    if (!detscat_str_grow(s, s->length + 2)) return false;
    s->data[s->length++] = c;
    s->data[s->length] = '\0';
    return true;
}

bool detscat_str_is_valid(const Str *s) {
    if (!s || !s->data) return false;
    if (s->capacity == 0 || s->length > s->capacity) return false;
    if (s->capacity > STR_MAX_CAP) return false;
    if (s->data[s->length] != '\0') return false;
    return true;
}
