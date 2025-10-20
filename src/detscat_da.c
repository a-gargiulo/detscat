#include "detscat_da.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


static bool detscat_da_grow(DetScatDa *da, size_t min_capacity) {
    if (!da || min_capacity == 0) return false;
    if (min_capacity > DA_MAX_CAP) return false;

    if (da->capacity >= min_capacity) return true;

    size_t new_capacity = da->capacity ? da->capacity : 1;
    while (new_capacity < min_capacity) {
        if (new_capacity > DA_MAX_CAP / 2) {
            new_capacity = DA_MAX_CAP;
            break;
        }
        new_capacity *= 2;
    }

    void *new_data = realloc(da->data, new_capacity * da->element_size);
    if (!new_data) return false;

    da->data = new_data;
    da->capacity = new_capacity;

    return true;
}

bool detscat_da_init(DetScatDa *da, size_t element_size, size_t initial_capacity) {
    if (!da || element_size == 0 || initial_capacity == 0) return false;

    da->data = malloc(initial_capacity * element_size);
    if (!da->data) {
        da->capacity = 0;
        da->length = 0;
        da->element_size = 0;
        return false;
    }

    da->element_size = element_size;
    da->length = 0;
    da->capacity = initial_capacity;

    return true;
}

bool detscat_da_append(DetScatDa *da, const void* value) {
    if (!da || !value) return false;
    if (!detscat_da_grow(da, da->length + 1)) return false;
    memcpy((unsigned char*)da->data + da->length * da->element_size, value, da->element_size);
    da->length++;
    return true;
}

void* detscat_da_get(DetScatDa *da, size_t index) {
    if (!da || index >= da->length) return NULL;
    return (unsigned char*)da->data + index * da->element_size;
}

bool detscat_da_set(DetScatDa *da, size_t index, const void *value) {
    if (!da || !value || index >= da->length) return false;
    memcpy((unsigned char*)da->data + index * da->element_size, value, da->element_size);
    return true;
}

bool detscat_da_reserve(DetScatDa *da, size_t needed_cap) {
    if (!da || needed_cap == 0) return false;
    return detscat_da_grow(da, needed_cap);
}

void detscat_da_free(DetScatDa *da) {
    if (!da) return;
    free(da->data);
    da->data = NULL;
    da->length = da->capacity = da->element_size = 0;
}
