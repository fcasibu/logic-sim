#include <string.h>

internal void
InitializeArena(memory_arena *arena, usize size, void *base)
{
    memory_block *first_block = (memory_block *)base;
    first_block->prev = 0;
    first_block->size = size;

    arena->current_block = first_block;
    arena->base = (u8 *)base + sizeof(*first_block);
    arena->used = 0;
    arena->capacity = size - sizeof(*first_block);
    arena->minimum_block_size = 0;
}

internal inline usize
GetAlignmentOffset(memory_arena *arena, usize alignment)
{
    usize result = (usize)arena->base + arena->used;
    usize mask = alignment - 1;
    return (alignment - (result & mask)) & mask;
}

internal inline usize
GetEffectiveSize(memory_arena *arena, usize size_init, usize alignment)
{
    return GetAlignmentOffset(arena, alignment) + size_init;
}

#define ZeroStruct(p) ZeroSize((p), sizeof(*(p)))
#define ZeroArray(count, p) ZeroSize((p), (count * sizeof(*(p))))

internal inline void
ZeroSize(void *ptr, usize size)
{
    memset(ptr, 0, size);
}

#define PushArray(a, count, type) (type *)PushSize_((a), (count) * sizeof(type), alignof(type))
#define PushStruct(a, type) (type *)PushSize_((a), sizeof(type), alignof(type))
#define PushSize(a, size) PushSize_((a), (size), alignof(max_align_t))

internal void *
PushSizeResize_(memory_arena *arena, usize size_init, usize alignment)
{
    if (!arena->minimum_block_size) {
        arena->minimum_block_size = MB(1);
    }

    usize header_size = sizeof(memory_block);
    usize size = GetEffectiveSize(arena, size_init, alignment);
    usize block_size = Max(size + header_size, arena->minimum_block_size);

    memory_block *new_block = Platform.AllocateMemory(block_size);
    Assert(new_block);

    new_block->prev = arena->current_block;
    new_block->size = block_size;

    arena->current_block = new_block;
    arena->base = (u8 *)new_block + header_size;
    arena->used = 0;
    arena->capacity = block_size - header_size;

    size = GetEffectiveSize(arena, size_init, alignment);
    Assert(arena->used + size <= arena->capacity);
    usize alignment_offset = GetAlignmentOffset(arena, alignment);
    void *result = arena->base + arena->used + alignment_offset;
    arena->used += size;

    Assert(size >= size_init);

    return result;
}

internal inline void *
PushSize_(memory_arena *arena, usize size_init, usize alignment)
{
    usize size = GetEffectiveSize(arena, size_init, alignment);

    if (arena->used + size <= arena->capacity) {
        Assert(arena->used + size <= arena->capacity);
        usize alignment_offset = GetAlignmentOffset(arena, alignment);
        void *result = arena->base + arena->used + alignment_offset;
        arena->used += size;

        Assert(size >= size_init);

        return result;
    }

    return PushSizeResize_(arena, size_init, alignment);
}

internal char *
PushString(memory_arena *arena, const char *source)
{
    usize size = strlen(source);
    char *dest = (char *)PushSize(arena, size + 1);
    memcpy(dest, source, size);
    dest[size] = '\0';

    return dest;
}

internal inline temporary_memory
BeginTemporaryMemory(memory_arena *arena)
{
    temporary_memory result = { 0 };

    result.arena = arena;
    result.used = arena->used;
    arena->temp_count += 1;

    return result;
}

internal inline void
EndTemporaryMemory(temporary_memory temp_mem)
{
    memory_arena *arena = temp_mem.arena;
    Assert(arena->used >= temp_mem.used);
    arena->used = temp_mem.used;
    Assert(arena->temp_count > 0);
    arena->temp_count -= 1;
}

internal void
ArenaReset(memory_arena *arena)
{
    while (arena->current_block->prev != 0) {
        memory_block *to_free = arena->current_block;
        memory_block *prev_block = to_free->prev;

        arena->current_block = prev_block;
        Platform.DeallocateMemory(to_free, to_free->size);
    }

    memory_block *root = arena->current_block;

    arena->base = (u8 *)root + sizeof(memory_block);
    arena->used = 0;
    arena->capacity = root->size - sizeof(memory_block);
    arena->temp_count = 0;
}

internal void
FreeArena(memory_arena *arena)
{
    memory_block *block = arena->current_block;
    while (block) {
        memory_block *prev = block->prev;
        Platform.DeallocateMemory(block, block->size);
        block = prev;
    }

    ZeroStruct(arena);
}

#define GrowArray(arena, da)                                                         \
    do {                                                                             \
        typedef typeof(*(da)->items) element_type;                                   \
        usize element_size = sizeof(element_type);                                   \
                                                                                     \
        usize old_cap = (da)->capacity;                                              \
        Assert(old_cap > 0);                                                         \
        usize new_cap = old_cap * 2;                                                 \
                                                                                     \
        u8 *arena_end = (u8 *)(arena)->base + (arena)->used;                         \
        u8 *array_end = (u8 *)(da)->items + (old_cap * element_size);                \
                                                                                     \
        if (arena_end == array_end) {                                                \
            usize growth = (new_cap - old_cap) * element_size;                       \
            if ((arena)->used + growth <= (arena)->capacity) {                       \
                (arena)->used += growth;                                             \
                (da)->capacity = new_cap;                                            \
            } else {                                                                 \
                element_type *new_items = PushArray((arena), new_cap, element_type); \
                memcpy(new_items, (da)->items, (da)->size * element_size);           \
                (da)->items = new_items;                                             \
                (da)->capacity = new_cap;                                            \
            }                                                                        \
        } else {                                                                     \
            element_type *new_items = PushArray((arena), new_cap, element_type);     \
            memcpy(new_items, (da)->items, (da)->size * element_size);               \
            (da)->items = new_items;                                                 \
            (da)->capacity = new_cap;                                                \
        }                                                                            \
    } while (0)

#define ArrayInit(arena, array, cap)                                     \
    do {                                                                 \
        (array)->items = PushArray(arena, cap, typeof(*(array)->items)); \
        (array)->capacity = cap;                                         \
        (array)->size = 0;                                               \
    } while (0)

#define ArrayPush(arena, array, item)              \
    do {                                           \
        if ((array)->size >= (array)->capacity)    \
            GrowArray((arena), (array));           \
        Assert((array)->size < (array)->capacity); \
        (array)->items[(array)->size++] = item;    \
    } while (0)
