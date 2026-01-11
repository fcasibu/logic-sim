#ifndef GAME_H
#define GAME_H

#define INPUT_BUF_SIZE 2048

typedef struct {
    b32 is_initialized;

    char input_buf[INPUT_BUF_SIZE];
    usize input_count;
    b32 input_active;
    char prev_buf[INPUT_BUF_SIZE];

    memory_arena main_arena;
    memory_arena result_arena;

    eval_result result;

    f32 scroll_y;
    usize selected_row;
} game_state;

#endif // GAME_H
