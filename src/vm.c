global vm VM;

internal u64
RunVM(u64 row_idx)
{
    chunk *c = VM.chunks;
    Assert(c);

    u8 *ip = c->items;
    u8 *end = c->items + c->size;
    u64 *sp = VM.stack;

    while (ip < end) {
        u8 opcode = *ip++;

        switch (opcode) {
            case OP_Var: {
                u8 var_idx = *ip++;

                u64 val;
                if (var_idx < 6) {
                    // a lot of wizardry found here https://graphics.stanford.edu/~seander/bithacks.html
                    local_const u64 b[] = {
                        0xAAAAAAAAAAAAAAAAULL, 0xCCCCCCCCCCCCCCCCULL, 0xF0F0F0F0F0F0F0F0ULL,
                        0xFF00FF00FF00FF00ULL, 0xFFFF0000FFFF0000ULL, 0xFFFFFFFF00000000ULL,
                    };
                    val = b[var_idx];
                } else {
                    val = ((row_idx >> var_idx) & 1) ? ~(u64)0 : 0;
                }
                *sp++ = val;
            } break;

            case OP_And: {
                u64 b = *--sp;
                u64 a = *--sp;
                *sp++ = a & b;
            } break;
            case OP_Or: {
                u64 b = *--sp;
                u64 a = *--sp;
                *sp++ = a | b;
            } break;
            case OP_Xor: {
                u64 b = *--sp;
                u64 a = *--sp;
                *sp++ = a ^ b;
            } break;
            case OP_Not: {
                u64 a = *--sp;
                *sp++ = ~a;
            } break;
            case OP_Imply: {
                u64 b = *--sp;
                u64 a = *--sp;
                *sp++ = (~a) | b;
            } break;
            case OP_Nand: {
                u64 b = *--sp;
                u64 a = *--sp;
                *sp++ = ~(a & b);
            } break;
            case OP_Nor: {
                u64 b = *--sp;
                u64 a = *--sp;
                *sp++ = ~(a | b);
            } break;
            case OP_Xnor: {
                u64 b = *--sp;
                u64 a = *--sp;
                *sp++ = ~(a ^ b);
            } break;
        }
    }

    return *(sp - 1);
}

internal truth_table *
GetTruthTable(memory_arena *arena)
{
    chunk *c = VM.chunks;
    Assert(c);

    truth_table *table = PushStruct(arena, truth_table);

    table->vars.capacity = c->vars.size;
    table->vars.size = c->vars.size;
    table->vars.items = PushArray(arena, table->vars.size, const char *);

    for (usize i = 0; i < c->vars.size; ++i)
        table->vars.items[i] = c->vars.items[i].name;

    table->row_count = (u64)1 << table->vars.size;
    usize chunk_count = (table->row_count + 63) / 64;
    ArrayInit(arena, &table->results, chunk_count);
    table->results.size = chunk_count;

    for (u64 i = 0; i < table->row_count; i += 64)
        table->results.items[i / 64] = RunVM(i);

    return table;
}

internal eval_result
Interpret(memory_arena *arena, const char *source)
{
    Assert(source);

    chunk c = { 0 };
    InitializeChunk(arena, &c, 1024);
    InitializeStringInternPool(arena, 31);

    VM.stack_top = VM.stack;
    VM.chunks = &c;
    VM.ip = VM.chunks->items;

    if (!Parse(arena, &c, source))
        return (eval_result){ Eval_ParseError, { NULL } };

    // TODO(fcasibu): simplification pass
    return (eval_result){ Eval_Ok, { GetTruthTable(arena) } };
}
