#ifndef ARENA_H
#define ARENA_H

typedef struct memory_block {
    struct memory_block *prev;
    usize size;
} memory_block;

typedef struct {
    memory_block *current_block;
    u8 *base;
    usize used;
    usize capacity;
    usize minimum_block_size;

    usize temp_count;
} memory_arena;

typedef struct {
    memory_arena *arena;
    usize used;
} temporary_memory;

#endif // ARENA_H
