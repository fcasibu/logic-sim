#ifndef VM_H
#define VM_H

typedef struct {
    const char **items;
    usize size;
    usize capacity;
} references;

typedef struct {
    u64 *items;
    usize size;
    usize capacity;
} results;

typedef struct {
    references vars;
    results results;

    usize row_count;
} truth_table;

typedef Enum(u8, eval_type){
    Eval_None,
    Eval_Ok,
    Eval_ParseError,
};

typedef struct {
    eval_type type;
    struct {
        truth_table *table;
    } value;
} eval_result;

#define STACK_MAX MB(1)

typedef struct {
    chunk *chunks;
    u8 *ip;

    u64 stack[STACK_MAX];
    u64 *stack_top;
} vm;

typedef struct {
    u16 value;
    u16 mask;
    b32 used;
} implicant;

typedef struct {
    implicant *items;
    usize size;
    usize capacity;
} implicants;

internal u8
GetTruthValue(const truth_table *table, u64 row_idx)
{
    Assert(row_idx < table->row_count);

    u64 chunk = table->results.items[row_idx / 64];
    return (u8)((chunk >> (row_idx % 64)) & 1);
}

#endif // VM_H
