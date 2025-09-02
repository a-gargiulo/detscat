#ifndef DETSCAT_STR_H
#define DETSCAT_STR_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} Str;

char *detscat_str_raw_trim(char *s);
char *detscat_str_raw_normpath(char *path);
char *detscat_str_raw_strdup(const char *s);
int detscat_str_raw_strcasecmp(const char *s1, const char *s2);

bool detscat_str_init(Str *s);
void detscat_str_free(Str *s);
bool detscat_str_reserve(Str *s, size_t needed_cap);
bool detscat_str_set(Str *s, const char *src);
bool detscat_str_copy(Str *dst, const Str *src);
bool detscat_str_append(Str *s, const char *suffix);
bool detscat_str_append_char(Str *s, char c);

#endif  // DETSCAT_STR_H
