#ifndef STR_H
#define STR_H

#include <stddef.h>
#include <stdbool.h>

char    *str_raw_trim(char *str);
char    *str_raw_normpath(char *path);
char    *str_raw_strdup(const char *str); 
int      str_raw_strcasecmp(const char *s1, const char *s2);

typedef struct {
    char    *data;
    size_t   length;
    size_t   capacity;
} Str;

bool    str_init(Str *s);
void    str_free(Str *s);
bool    str_reserve(Str *s, size_t new_cap);
bool    str_set(Str *s, const char *src);
bool    str_copy(Str *dst, const Str *src);
bool    str_append(Str *s, const char *suffix);
bool    str_append_char(Str *s, char c); 

#endif  // STRUTIL_H
