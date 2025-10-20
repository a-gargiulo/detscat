#ifndef DETSCAT_DA
#define DETSCAT_DA

#include <stddef.h>
#include <stdbool.h>

#define DA_MAX_CAP (1UL << 30)

typedef struct {
    void *data;
    size_t element_size;
    size_t length;
    size_t capacity;
} DetScatDa;

bool detscat_da_init(DetScatDa *da, size_t element_size, size_t initial_capacity);

bool detscat_da_append(DetScatDa *da, const void* value); 

void* detscat_da_get(DetScatDa *da, size_t index); 

bool detscat_da_set(DetScatDa *da, size_t index, const void *value);

bool detscat_da_reserve(DetScatDa *da, size_t needed_cap); 

void detscat_da_free(DetScatDa *da);

#endif  // DETSCAT_DA
