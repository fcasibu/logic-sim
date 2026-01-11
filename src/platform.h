#ifndef PLATFORM_H
#define PLATFORM_H

#define ALLOCATE_MEMORY(name) void *name(usize size)
typedef ALLOCATE_MEMORY(platform_allocate_memory);

#define DEALLOCATE_MEMORY(name) void name(void *mem, usize size)
typedef DEALLOCATE_MEMORY(platform_deallocate_memory);

typedef struct {
    platform_allocate_memory *AllocateMemory;
    platform_deallocate_memory *DeallocateMemory;
} platform_api;

global platform_api Platform;

typedef struct {
    void *permanent_storage;
    usize permanent_storage_size;

    usize temporary_storage_size;
    void *temporary_storage;

    int width;
    int height;

    b32 has_error;
    platform_api platform;
} context;

#define GAME_UPDATE_AND_RENDER(name) void name(context *ctx)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#endif // PLATFORM_H
