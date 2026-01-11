#include <sys/mman.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <raylib.h>

#include "base.h"
#include "platform.h"

typedef struct {
    void *game_code_handle;
    i64 last_write_time;

    game_update_and_render *UpdateAndRender;
} game_code;

internal i64
GetFileTimestamp(const char *filename)
{
    struct stat result;
    if (stat(filename, &result) == 0)
        return result.st_mtimespec.tv_sec * 1000000000LL + result.st_mtimespec.tv_nsec;

    return 0;
}

internal game_code
LoadGameCode(const char *source_lib_path)
{
    game_code result = { 0 };
    result.game_code_handle = dlopen(source_lib_path, RTLD_NOW);
    result.last_write_time = GetFileTimestamp(source_lib_path);

    if (!result.game_code_handle) {
        fprintf(stderr, "Library could not be loadead: %s", dlerror());

        return result;
    }

    result.UpdateAndRender =
        (game_update_and_render *)dlsym(result.game_code_handle, "GameUpdateAndRender");

    Assert(result.UpdateAndRender);

    return result;
}

internal void
UnloadGameCode(game_code *code)
{
    Assert(code);

    if (code->game_code_handle) {
        dlclose(code->game_code_handle);
        code->game_code_handle = NULL;
    }

    code->UpdateAndRender = NULL;
}

internal void
ReloadGameCode(game_code *code, const char *source_lib_path)
{
    Assert(code);

    if (code->UpdateAndRender) {
        UnloadGameCode(code);
        *code = LoadGameCode(source_lib_path);
    }
}

internal
ALLOCATE_MEMORY(AllocateMemory)
{
    void *result = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

    if (result == MAP_FAILED)
        return NULL;

    return result;
}

internal
DEALLOCATE_MEMORY(DeallocateMemory)
{
    if (mem) {
        munmap(mem, size);
    }
}

internal void
InitializeContext(context *ctx)
{
    Assert(ctx);

    ctx->permanent_storage_size = MB(512);
    ctx->temporary_storage_size = MB(512);
    usize total_size = ctx->permanent_storage_size + ctx->temporary_storage_size;

    ctx->permanent_storage = AllocateMemory(total_size);
    ctx->temporary_storage = (u8 *)ctx->permanent_storage + ctx->permanent_storage_size;
    ctx->platform.AllocateMemory = AllocateMemory;
    ctx->platform.DeallocateMemory = DeallocateMemory;
}

int
main(void)
{
    context ctx = { 0 };
    InitializeContext(&ctx);

    Platform = ctx.platform;

    const char *source_lib_path = "./build/game.so";
    game_code code = LoadGameCode(source_lib_path);

    ctx.width = 1280;
    ctx.height = 720;

    InitWindow(ctx.width, ctx.height, "logic sim");
    SetTargetFPS(60);

    // TODO(fcasibu): gate visualization
    while (!WindowShouldClose()) {
        if (code.last_write_time < GetFileTimestamp(source_lib_path))
            ReloadGameCode(&code, source_lib_path);

        if (code.UpdateAndRender)
            code.UpdateAndRender(&ctx);
    }

    CloseWindow();
    DeallocateMemory(ctx.permanent_storage,
                     ctx.permanent_storage_size + ctx.temporary_storage_size);

    return 0;
}
