#include <stdio.h>

#include "base.h"
#include "arena.h"

typedef struct {
    memory_arena main_arena;
    memory_arena temporary_arena;
} context;

#include "intern.h"
#include "lexer.h"
#include "node.h"
#include "parser.h"

#include "arena.c"
#include "intern.c"
#include "lexer.c"
#include "node.c"
#include "parser.c"

internal void
InitializeContext(context *ctx)
{
    Assert(ctx);

    usize permanent_storage_size = MB(64);
    usize temporary_storage_size = MB(512);
    usize total_size = permanent_storage_size + temporary_storage_size;

    u8 *permanent_storage =
        (u8 *)mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    Assert(permanent_storage);

    u8 *temporary_storage = (permanent_storage + permanent_storage_size);

    InitializeArena(&ctx->main_arena, permanent_storage_size, permanent_storage);
    InitializeArena(&ctx->temporary_arena, temporary_storage_size, temporary_storage);
}

int
main(void)
{
    context ctx = { 0 };
    InitializeContext(&ctx);

    const char *source = "(A OR B) AND NOT C";
    ast_nodes nodes = { 0 };
    InitializeNodes(&ctx.main_arena, &nodes, 16);
    InitializeStringInternPool(&ctx.main_arena, 16);

    if (!Parse(&ctx.main_arena, &nodes, source))
        return 1;

    PrintAst(&nodes);
    // TODO(fcasibu): interpreter/ui

    FreeArena(&ctx.main_arena);

    return 0;
}
