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
    Assert(c && c->vars.size <= 10);

    truth_table *table = PushStruct(arena, typeof(*table));
    table->vars.size = c->vars.size;
    table->vars.items = PushArray(arena, table->vars.size, typeof(*table->vars.items));

    for (usize i = 0; i < table->vars.size; ++i)
        table->vars.items[i] = c->vars.items[i].name;

    table->row_count = 1 << table->vars.size;

    table->results.size = (table->row_count + 63) / 64;
    ArrayInit(arena, &table->results, table->results.size);

    for (usize i = 0; i < table->row_count; i += 64)
        table->results.items[i / 64] = RunVM(i);

    return table;
}

internal inline b32
TryMergeImplicants(implicant a, implicant b, implicant *out)
{
    if (a.mask != b.mask)
        return false;

    u16 diff = a.value ^ b.value;

    if (diff && ((diff & (diff - 1)) == 0)) {
        out->value = a.value & ~diff;
        out->mask = a.mask | diff;
        out->used = false;
        return true;
    }

    return false;
}

internal inline b32
ImplicantCovers(implicant imp, u16 minterm)
{
    return (minterm & ~imp.mask) == imp.value;
}

// https://en.wikipedia.org/wiki/Quine%E2%80%93McCluskey_algorithm
internal implicants *
FindPrimeImplicants(memory_arena *arena, const truth_table *table)
{
    Assert(table);

    implicants current = { 0 };
    ArrayInit(arena, &current, table->row_count);

    for (usize i = 0; i < table->row_count; ++i) {
        if (GetTruthValue(table, i)) {
            implicant imp = { .value = (u16)i, .mask = 0, .used = false };
            ArrayPush(arena, &current, imp);
        }
    }

    implicants primes = { 0 };
    ArrayInit(arena, &primes, current.size);

    while (current.size > 0) {
        implicants next = { 0 };
        ArrayInit(arena, &next, current.size);
        b32 merged_any = false;

        for (usize i = 0; i < current.size; ++i) {
            for (usize j = i + 1; j < current.size; ++j) {
                implicant merged;

                if (TryMergeImplicants(current.items[i], current.items[j], &merged)) {
                    current.items[i].used = true;
                    current.items[j].used = true;
                    merged_any = true;

                    b32 exists = false;
                    for (usize k = 0; k < next.size; ++k) {
                        if (next.items[k].value == merged.value &&
                            next.items[k].mask == merged.mask) {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists)
                        ArrayPush(arena, &next, merged);
                }
            }
        }

        for (usize i = 0; i < current.size; ++i) {
            if (!current.items[i].used)
                ArrayPush(arena, &primes, current.items[i]);
        }

        current = next;
        if (!merged_any)
            break;
    }

    implicants *essentials = PushStruct(arena, typeof(*essentials));
    ArrayInit(arena, essentials, primes.size);

    for (usize i = 0; i < table->row_count; ++i) {
        if (!GetTruthValue(table, i))
            continue;

        u16 minterm = (u16)i;
        usize cover_count = 0;
        usize last_idx = 0;

        for (usize j = 0; j < primes.size; ++j) {
            if (ImplicantCovers(primes.items[j], minterm)) {
                cover_count++;
                last_idx = j;
            }
        }

        if (cover_count == 1 && !primes.items[last_idx].used) {
            primes.items[last_idx].used = true;
            ArrayPush(arena, essentials, primes.items[last_idx]);
        }
    }

    if (essentials->size == 0 && primes.size > 0) {
        for (usize i = 0; i < primes.size; ++i)
            ArrayPush(arena, essentials, primes.items[i]);
    }

    return essentials;
}

internal const char *
FormatImplicant(memory_arena *arena, const truth_table *table, implicant imp)
{
    char *result = PushSize(arena, 256);
    usize pos = 0;
    b32 first = true;

    for (usize i = 0; i < table->vars.size; ++i) {
        if (!((imp.mask >> i) & 1)) {
            if (!first)
                pos += snprintf(result + pos, 256 - pos, " AND ");

            first = false;
            if (!((imp.value >> i) & 1))
                pos += snprintf(result + pos, 256 - pos, "NOT ");

            pos += snprintf(result + pos, 256 - pos, "%s", table->vars.items[i]);
        }
    }

    return result;
}

internal const char *
TrySimplifyGate(memory_arena *arena, const truth_table *table, const implicants *imps)
{
    Assert(table);
    Assert(imps);

    u16 active_vars_mask = 0;
    u16 all_mask = (1 << table->vars.size) - 1;

    for (usize i = 0; i < imps->size; ++i)
        active_vars_mask |= (~imps->items[i].mask) & all_mask;

    usize var_count = 0;
    usize idx[2] = { 0 };
    for (usize i = 0; i < table->vars.size; ++i) {
        if ((active_vars_mask >> i) & 1) {
            if (var_count < 2)
                idx[var_count] = i;
            var_count++;
        }
    }

    if (var_count != 2)
        return NULL;

    u8 sig = 0;
    for (usize i = 0; i < 4; ++i) {
        u16 a = (i >> 0) & 1;
        u16 b = (i >> 1) & 1;

        for (usize k = 0; k < imps->size; ++k) {
            implicant imp = imps->items[k];
            if (!((imp.mask >> idx[0]) & 1) && (((imp.value >> idx[0]) & 1) != a))
                continue;
            if (!((imp.mask >> idx[1]) & 1) && (((imp.value >> idx[1]) & 1) != b))
                continue;

            sig |= (1 << i);
            break;
        }
    }

    const char *nA = table->vars.items[idx[0]];
    const char *nB = table->vars.items[idx[1]];
    char *buf = PushSize(arena, 256);

    switch (sig) {
        case 0x1:
            snprintf(buf, 256, "%s NOR %s", nA, nB);
            break;
        case 0x6:
            snprintf(buf, 256, "%s XOR %s", nA, nB);
            break;
        case 0x7:
            snprintf(buf, 256, "%s NAND %s", nA, nB);
            break;
        case 0x8:
            snprintf(buf, 256, "%s AND %s", nA, nB);
            break;
        case 0x9:
            snprintf(buf, 256, "%s XNOR %s", nA, nB);
            break;
        case 0xB:
            snprintf(buf, 256, "%s IMPLY %s", nB, nA);
            break;
        case 0xD:
            snprintf(buf, 256, "%s IMPLY %s", nA, nB);
            break;
        case 0xE:
            snprintf(buf, 256, "%s OR %s", nA, nB);
            break;
        default:
            return NULL;
    }

    return buf;
}

// TODO(fcasibu): odd number of signals
internal const char *
SimplifyExpression(memory_arena *arena, const truth_table *table)
{
    Assert(table);

    implicants *essentials = FindPrimeImplicants(arena, table);
    Assert(essentials);

    if (essentials->size == 0) {
        char *result = PushSize(arena, 2048);
        snprintf(result, 2048, "%s AND NOT %s", table->vars.items[0], table->vars.items[0]);
        return result;
    }

    u16 all_mask = (1 << table->vars.size) - 1;
    if (essentials->size == 1 && essentials->items[0].mask == all_mask) {
        char *result = PushSize(arena, 2048);
        snprintf(result, 2048, "%s OR NOT %s", table->vars.items[0], table->vars.items[0]);
        return result;
    }

    const char *simple = TrySimplifyGate(arena, table, essentials);
    if (simple)
        return simple;

    u16 common_mask = all_mask;
    u16 common_value = 0;

    for (usize i = 0; i < essentials->size; ++i) {
        u16 fixed_bits = ~essentials->items[i].mask & all_mask;
        if (i == 0) {
            common_mask = fixed_bits;
            common_value = essentials->items[i].value & fixed_bits;
        } else {
            u16 both_fixed = common_mask & fixed_bits;
            u16 same_vals = ~(common_value ^ essentials->items[i].value);
            common_mask = both_fixed & same_vals;
            common_value &= common_mask;
        }
    }

    if (common_mask != 0) {
        implicants reduced = { 0 };
        ArrayInit(arena, &reduced, essentials->size);

        for (usize i = 0; i < essentials->size; ++i) {
            implicant imp = { .value = essentials->items[i].value & ~common_mask,
                              .mask = essentials->items[i].mask | common_mask,
                              .used = false };
            ArrayPush(arena, &reduced, imp);
        }

        const char *inner = TrySimplifyGate(arena, table, &reduced);

        if (inner) {
            char *factor = PushSize(arena, 1024);
            usize fpos = 0;
            b32 first = true;

            for (usize i = 0; i < table->vars.size; ++i) {
                if ((common_mask >> i) & 1) {
                    if (!first)
                        fpos += snprintf(factor + fpos, 1024 - fpos, " AND ");
                    first = false;

                    if (!((common_value >> i) & 1))
                        fpos += snprintf(factor + fpos, 1024 - fpos, "NOT ");

                    fpos += snprintf(factor + fpos, 1024 - fpos, "%s", table->vars.items[i]);
                }
            }

            char *result = PushSize(arena, 2048);
            snprintf(result, 2048, "(%s) AND (%s)", inner, factor);
            return result;
        }
    }

    char *result = PushSize(arena, 2048);
    usize pos = 0;

    for (usize i = 0; i < essentials->size; ++i) {
        if (i > 0)
            pos += snprintf(result + pos, 2048 - pos, " OR ");

        const char *term = FormatImplicant(arena, table, essentials->items[i]);

        if (essentials->size > 1 && strstr(term, " AND ")) {
            pos += snprintf(result + pos, 2048 - pos, "(%s)", term);
        } else {
            pos += snprintf(result + pos, 2048 - pos, "%s", term);
        }
    }

    return result;
}

internal eval_result
Interpret(memory_arena *arena, const char *source)
{
    Assert(source);

    chunk c = { 0 };
    InitializeChunk(arena, &c, 2048);
    InitializeStringInternPool(arena, 10);

    VM.stack_top = VM.stack;
    VM.chunks = &c;
    VM.ip = VM.chunks->items;

    if (!Parse(arena, &c, source))
        return (eval_result){ Eval_ParseError, { NULL } };

    return (eval_result){ Eval_Ok, { GetTruthTable(arena) } };
}
