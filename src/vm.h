#ifndef VM_H
#define VM_H

// TODO(fcasibu): hashmap
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

    u64 row_count;
} truth_table;

typedef Enum(u8, eval_type){
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

internal u8
GetTruthValue(truth_table *table, u64 row_idx)
{
    Assert(row_idx < table->row_count);

    u64 chunk = table->results.items[row_idx / 64];
    return (u8)((chunk >> (row_idx % 64)) & 1);
}

#endif // VM_H
