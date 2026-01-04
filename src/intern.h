#ifndef INTERN_H
#define INTERN_H

// TODO(fcasibu): Hashmap
typedef struct {
    const char **items;
    usize size;
    usize capacity;

    memory_arena *arena;
} string_intern_array;

#endif // INTERN_H
