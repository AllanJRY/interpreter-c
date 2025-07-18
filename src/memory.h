#ifndef INTERP_MEMORY_H

// TODO(AJ): Use own built allocator to avoid using stdlib realloc.
// TODO(AJ): prefix everything with `mem_`.

#include "common.h"
#include "object.h"

#define ALLOCATE(type, count) (type*) reallocate(NULL, 0, sizeof(type) * count)

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, old_capacity, new_capacity) (type*) reallocate(pointer, sizeof(type) * (old_capacity), sizeof(type) * (new_capacity))

#define FREE_ARRAY(type, pointer, old_capacity) reallocate(pointer, sizeof(type) * (old_capacity), 0)


void* reallocate(void* pointer, size_t old_size, size_t new_size);

void mem_free_objects(void);

#define INTERP_MEMORY_H
#endif
