global string_intern_array StringInternArray;

internal void
InitializeStringInternPool(memory_arena *arena, usize initial_cap)
{
    StringInternArray.capacity = initial_cap;
    StringInternArray.size = 0;
    StringInternArray.items = PushArray(arena, initial_cap, typeof(*StringInternArray.items));
    StringInternArray.arena = arena;

    Assert(StringInternArray.items);
}

internal const char *
InternString(const char *str)
{
    for (usize i = 0; i < StringInternArray.size; ++i) {
        if (strcmp(StringInternArray.items[i], str) == 0)
            return StringInternArray.items[i];
    }

    if (StringInternArray.size >= StringInternArray.capacity)
        GrowArray(StringInternArray.arena, &StringInternArray);

    Assert(StringInternArray.size < StringInternArray.capacity);
    char *result = PushString(StringInternArray.arena, str);
    StringInternArray.items[StringInternArray.size++] = result;

    return result;
}
