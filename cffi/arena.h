#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

#ifndef MAX_ARENA_MEMORY
#define MAX_ARENA_MEMORY 2 << 20
#endif

// Allocate memory in the arena. It is guaranteed to success of exit(1)
// This memory should not be freed directly.
// Not thread safe.
void* ARENA_ALLOC(size_t size);

// Duplicate string, allocating it in arena.
char* ARENA_STRDUP(const char* s);

#endif /* ARENA_H */
