internal void
InitializeVars(memory_arena *arena, vars *v, usize initial_cap)
{
    Assert(arena);
    Assert(v);

    v->capacity = initial_cap;
    v->size = 0;
    v->items = PushArray(arena, initial_cap, typeof(*v->items));

    Assert(v->items);
}

internal void
InitializeChunk(memory_arena *arena, chunk *c, usize initial_cap)
{
    Assert(arena);
    Assert(c);

    c->capacity = initial_cap;
    c->size = 0;
    c->items = PushArray(arena, initial_cap, typeof(*c->items));
    Assert(c->items);

    InitializeVars(arena, &c->vars, 10);
}

internal inline void
WriteChunk(memory_arena *arena, chunk *c, u8 byte)
{
    Assert(arena);
    Assert(c);

    if (c->size >= c->capacity)
        GrowArray(arena, c);

    Assert(c->size < c->capacity);
    c->items[c->size++] = byte;
}

internal inline usize
AddVar(memory_arena *arena, chunk *c, const char *name, usize intern_idx)
{
    Assert(arena);
    Assert(c);

    for (usize i = 0; i < c->vars.size; ++i) {
        if (c->vars.items[i].name == name)
            return i;
    }

    if (c->vars.size >= c->vars.capacity)
        GrowArray(arena, &c->vars);

    c->vars.items[c->vars.size].name = name;
    c->vars.items[c->vars.size].index = intern_idx;
    return c->vars.size++;
}

internal inline void
WriteVar(memory_arena *arena, chunk *c, const char *name, usize idx)
{
    Assert(arena);
    Assert(c);

    usize var_idx = AddVar(arena, c, name, idx);
    Assert((var_idx + 1) <= 10);
    WriteChunk(arena, c, OP_Var);
    WriteChunk(arena, c, var_idx);
}
