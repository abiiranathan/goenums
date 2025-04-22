#include "arena.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Statically allocate 2 MB
static unsigned char memory[MAX_ARENA_MEMORY] = {0};
static size_t allocated                       = 0;

void* ARENA_ALLOC(size_t size) {
    if (allocated + size > MAX_ARENA_MEMORY) {
        fprintf(stderr, "OOM\n");
        exit(1);
    }

    unsigned char* ptr = memory + allocated;
    allocated += size;
    return ptr;
}

// Duplicate string, allocating it in arena.
char* ARENA_STRDUP(const char* s) {
    if (s == NULL) {
        puts("ARENA_STRDUP(): s is NULL\n");
        exit(1);
    }

    size_t len = strlen(s);
    char* ptr  = ARENA_ALLOC(len + 1);
    ptr[len]   = '\0';
    strcpy(ptr, s);  // we are sure to succeed.
    return ptr;
}
