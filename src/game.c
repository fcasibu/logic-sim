#include <raylib.h>
#include <string.h>
#include <ctype.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "base.h"
#include "arena.h"
#include "platform.h"

#include "intern.h"
#include "lexer.h"
#include "chunk.h"
#include "compiler.h"
#include "vm.h"
#include "game.h"

#include "arena.c"
#include "intern.c"
#include "lexer.c"
#include "chunk.c"
#include "compiler.c"
#include "vm.c"

#define TOOLBAR_H 60.0f
#define ROW_H 30.0f

internal void
RunEvaluation(context *ctx, game_state *state)
{
    ArenaReset(&state->result_arena);

    state->input_count = strlen(state->input_buf);
    state->result = Interpret(&state->result_arena, state->input_buf);

    if (state->result.type == Eval_Ok) {
        state->selected_row = 0;
        state->scroll_y = 0;
        ctx->has_error = false;
    } else {
        ctx->has_error = true;
    }
}

internal void
DrawTruthTableUI(context *ctx, game_state *state, Vector2 m)
{
    truth_table *table = state->result.value.table;

    if (!state->input_active) {
        state->scroll_y -= GetMouseWheelMove() * (ROW_H * 3);
        if (state->scroll_y < 0)
            state->scroll_y = 0;

        f32 max_scroll = (table->row_count * ROW_H) - (ctx->height - TOOLBAR_H - 40);
        if (state->scroll_y > max_scroll && max_scroll > 0)
            state->scroll_y = max_scroll;

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && m.y < ctx->height - TOOLBAR_H) {
            usize clicked = ((m.y - 40 + state->scroll_y) / ROW_H);
            if (clicked < table->row_count)
                state->selected_row = clicked;
        }
    }

    BeginScissorMode(0, 40, ctx->width, ctx->height - TOOLBAR_H - 40);
    usize start = (state->scroll_y / ROW_H);
    usize end = start + (ctx->height / ROW_H) + 2;

    for (usize i = start; i < end && i < table->row_count; ++i) {
        f32 y = 40 + (i * ROW_H - state->scroll_y);
        if (i == state->selected_row)
            DrawRectangle(0, y, ctx->width, ROW_H, DARKGRAY);

        for (usize k = 0; k < table->vars.size; ++k) {
            u8 bit = (i >> k) & 1;
            DrawText(TextFormat("%d", bit), 15 + (k * 35), y + 7, 16, bit ? LIME : GRAY);
        }

        u8 val = GetTruthValue(table, i);
        DrawText(val ? "1" : "0", 25 + (table->vars.size * 35), y + 7, 16, val ? LIME : RED);
    }
    EndScissorMode();

    DrawRectangle(0, 0, ctx->width, 40, BLACK);
    for (usize k = 0; k < table->vars.size; ++k)
        DrawText(table->vars.items[k], 15 + (k * 35), 10, 16, WHITE);

    DrawText("RESULT", 25 + (table->vars.size * 35), 10, 16, GOLD);
}

internal void
HandleInputShortcuts(game_state *state)
{
    if (!state->input_active)
        return;

    b32 ctrl = IsKeyDown(KEY_LEFT_SUPER);

    if (ctrl && IsKeyPressed(KEY_C)) {
        SetClipboardText(state->input_buf);
    }

    if (ctrl && IsKeyPressed(KEY_X)) {
        SetClipboardText(state->input_buf);
        memset(state->input_buf, 0, INPUT_BUF_SIZE);
        state->input_count = 0;
    }

    if (ctrl && IsKeyPressed(KEY_V)) {
        const char *clip = GetClipboardText();

        if (clip) {
            usize current_len = strlen(state->input_buf);
            usize clip_len = strlen(clip);

            if (current_len + clip_len < INPUT_BUF_SIZE) {
                strcat(state->input_buf, clip);

                for (usize i = 0; state->input_count; i++)
                    state->input_buf[i] = toupper(state->input_buf[i]);
            }
        }
    }

    if (ctrl && IsKeyPressed(KEY_A)) {
        memset(state->input_buf, 0, INPUT_BUF_SIZE);
        state->input_count = 0;
    }
}

// man ui sucks, starting to appreciate html+css now
extern GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    game_state *state = (game_state *)ctx->permanent_storage;

    if (!state->is_initialized) {
        InitializeArena(&state->main_arena,
                        ctx->permanent_storage_size - sizeof(game_state),
                        (u8 *)ctx->permanent_storage + sizeof(game_state));

        InitializeArena(
            &state->result_arena, ctx->temporary_storage_size, (u8 *)ctx->temporary_storage);

        state->input_active = false;
        state->is_initialized = true;
        state->input_count = 0;

        GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
        GuiSetStyle(DEFAULT, BORDER_WIDTH, 2);
    }

    temporary_memory temp_mem = BeginTemporaryMemory(&state->main_arena);
    Vector2 m = GetMousePosition();

    Rectangle r_input = { 20, ctx->height - 45, 950, 30 };
    Rectangle r_eval = { 985, ctx->height - 45, 137, 30 };
    Rectangle r_simp = { 985 + 147, ctx->height - 45, 137, 30 };

    HandleInputShortcuts(state);

    BeginDrawing();
    ClearBackground(BLACK);

    DrawText("PREV: ", 20, r_input.y - 20, 14, GRAY);
    DrawText(state->prev_buf, 20 + MeasureText("PREV: ", 14), r_input.y - 20, 14, GOLD);

    if (state->result.type == Eval_Ok)
        DrawTruthTableUI(ctx, state, m);

    if (ctx->has_error)
        DrawText("PARSE ERROR", ctx->width - MeasureText("PARSE ERROR", 20) - 20, 10, 20, RED);

    Color border_color = state->input_active ? WHITE : (ctx->has_error ? RED : DARKGRAY);
    GuiSetStyle(TEXTBOX, BORDER_COLOR_NORMAL, ColorToInt(border_color));
    GuiSetStyle(TEXTBOX, BORDER_COLOR_FOCUSED, ColorToInt(border_color));
    GuiSetStyle(TEXTBOX, BORDER_COLOR_PRESSED, ColorToInt(border_color));
    GuiSetStyle(TEXTBOX, BASE_COLOR_NORMAL, ColorToInt(BLACK));
    GuiSetStyle(TEXTBOX, BASE_COLOR_FOCUSED, ColorToInt(BLACK));
    GuiSetStyle(TEXTBOX, BASE_COLOR_PRESSED, ColorToInt(BLACK));
    GuiSetStyle(TEXTBOX, TEXT_COLOR_NORMAL, ColorToInt(WHITE));
    GuiSetStyle(TEXTBOX, TEXT_COLOR_FOCUSED, ColorToInt(WHITE));
    GuiSetStyle(TEXTBOX, TEXT_COLOR_PRESSED, ColorToInt(WHITE));

    if (GuiTextBox(r_input, state->input_buf, INPUT_BUF_SIZE, state->input_active))
        state->input_active = !state->input_active;

    for (usize i = 0; state->input_buf[i]; i++)
        state->input_buf[i] = toupper(state->input_buf[i]);

    GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, ColorToInt(BLACK));

    b32 pressed_eval = GuiButton(r_eval, "EVAL");
    if (pressed_eval) {
        memset(state->prev_buf, 0, INPUT_BUF_SIZE);
        strncpy(state->prev_buf, state->input_buf, INPUT_BUF_SIZE - 1);

        RunEvaluation(ctx, state);
    }

    if (GuiButton(r_simp, "SIMPLIFY") && state->result.type == Eval_Ok) {
        memset(state->prev_buf, 0, INPUT_BUF_SIZE);
        strncpy(state->prev_buf, state->input_buf, INPUT_BUF_SIZE - 1);

        const char *simp = SimplifyExpression(temp_mem.arena, state->result.value.table);

        memset(state->input_buf, 0, INPUT_BUF_SIZE);
        usize len = strlen(simp);
        if (len >= INPUT_BUF_SIZE)
            len = INPUT_BUF_SIZE - 1;

        strncpy(state->input_buf, simp, len);

        RunEvaluation(ctx, state);
    }

    EndDrawing();
    EndTemporaryMemory(temp_mem);
}
