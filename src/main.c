#include <stdio.h>

#include <raylib.h>

#include "base.h"
#include "arena.h"

typedef struct {
    memory_arena main_arena;
    memory_arena temporary_arena;
} context;

#include "intern.h"
#include "lexer.h"
#include "chunk.h"
#include "compiler.h"
#include "vm.h"

#include "arena.c"
#include "intern.c"
#include "lexer.c"
#include "chunk.c"
#include "compiler.c"
#include "vm.c"

internal void
InitializeContext(context *ctx)
{
    Assert(ctx);

    usize permanent_storage_size = MB(512);
    usize total_size = permanent_storage_size;

    u8 *permanent_storage =
        (u8 *)mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    Assert(permanent_storage);

    InitializeArena(&ctx->main_arena, permanent_storage_size, permanent_storage);
}

int
main(void)
{
    context ctx = { 0 };
    InitializeContext(&ctx);

    char input_buf[1024] = "(A OR B) AND C";
    usize input_count = strlen(input_buf);
    b32 input_active = false;
    b32 has_error = false;

    const int screen_w = 1280;
    const int screen_h = 720;
    const f32 toolbar_h = 60.0f;
    const f32 row_h = 30.0f;

    InitWindow(screen_w, screen_h, "logic sim");
    SetTargetFPS(60);

    temporary_memory temp_mem = BeginTemporaryMemory(&ctx.main_arena);
    // TODO(fcasibu): gate visualization
    eval_result result = Interpret(temp_mem.arena, input_buf);

    f64 scroll_y = 0.0;
    u64 selected_row = 0;

    while (!WindowShouldClose()) {
        Vector2 m = GetMousePosition();
        Rectangle input_rect = { 20, screen_h - 45, 950, 30 };
        Rectangle btn_rect = { 985, screen_h - 45, 275, 30 };

        if (CheckCollisionPointRec(m, input_rect))
            SetMouseCursor(MOUSE_CURSOR_IBEAM);
        else
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            input_active = CheckCollisionPointRec(m, input_rect);
        }

        if (input_active) {
            int key = GetCharPressed();
            while (key > 0) {
                if ((key >= 32) && (key <= 125) && (input_count < 1023)) {
                    input_buf[input_count++] = (char)key;
                    input_buf[input_count] = '\0';
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && input_count > 0)
                input_buf[--input_count] = '\0';
        }

        if (IsKeyPressed(KEY_ENTER) ||
            (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(m, btn_rect))) {
            EndTemporaryMemory(temp_mem);
            temp_mem = BeginTemporaryMemory(&ctx.main_arena);

            result = Interpret(temp_mem.arena, input_buf);
            if (result.type == Eval_Ok) {
                selected_row = 0;
                scroll_y = 0;
                has_error = false;
            } else {
                has_error = true;
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);

        if (result.type == Eval_Ok) {
            truth_table *table = result.value.table;

            if (!input_active) {
                scroll_y -= GetMouseWheelMove() * (row_h * 3);
                if (scroll_y < 0)
                    scroll_y = 0;
                double max_s = ((double)table->row_count * row_h) - (screen_h - toolbar_h - 40);
                if (scroll_y > max_s && max_s > 0)
                    scroll_y = max_s;

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && m.y < screen_h - toolbar_h) {
                    u64 clicked = (u64)((m.y - 40 + scroll_y) / row_h);
                    if (clicked < table->row_count)
                        selected_row = clicked;
                }
            }

            BeginScissorMode(0, 40, screen_w, screen_h - toolbar_h - 40);
            u64 start = (u64)(scroll_y / row_h);
            u64 end = start + (u64)(screen_h / row_h) + 2;
            for (u64 i = start; i < end; ++i) {
                if (i >= table->row_count)
                    break;
                float y = 40 + (float)((double)i * row_h - scroll_y);
                if (i == selected_row)
                    DrawRectangle(0, y, screen_w, row_h, DARKGRAY);

                for (usize k = 0; k < table->vars.size; ++k) {
                    b32 b = (i >> k) & 1;
                    DrawText(TextFormat("%d", b), 15 + (k * 35), y + 7, 16, b ? LIME : GRAY);
                }
                u8 res_val = GetTruthValue(table, i);
                DrawText(res_val ? "1" : "0",
                         25 + (table->vars.size * 35),
                         y + 7,
                         16,
                         res_val ? LIME : RED);
            }
            EndScissorMode();

            DrawRectangle(0, 0, screen_w, 40, BLACK);
            for (usize k = 0; k < table->vars.size; ++k) {
                DrawText(table->vars.items[k], 15 + (k * 35), 10, 16, WHITE);
            }
            DrawText("RESULT", 25 + (table->vars.size * 35), 10, 16, GOLD);
        }

        DrawRectangleRec(input_rect, BLACK);
        DrawRectangleLinesEx(input_rect, 2, input_active ? LIME : (has_error ? RED : DARKGRAY));
        DrawText(input_buf, input_rect.x + 10, input_rect.y + 7, 18, RAYWHITE);
        if (input_active && (int)(GetTime() * 2) % 2 == 0)
            DrawRectangle(
                input_rect.x + 12 + MeasureText(input_buf, 18), input_rect.y + 6, 2, 18, LIME);

        DrawRectangleRec(btn_rect, CheckCollisionPointRec(m, btn_rect) ? DARKGREEN : GREEN);
        int text_len = MeasureText("EVAL", 18);
        DrawText("EVAL", btn_rect.x + ((btn_rect.width - text_len) / 2), btn_rect.y + 7, 18, BLACK);

        if (has_error)
            DrawText("PARSE ERROR", 1000, 10, 20, RED);

        EndDrawing();
    }

    CloseWindow();
    FreeArena(&ctx.main_arena);

    return 0;
}
