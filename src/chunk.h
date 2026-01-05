#ifndef CHUNK_H
#define CHUNK_H

// clang-format off
typedef Enum(u8, op_code){
    OP_Var, OP_And, OP_Or, OP_Xor, OP_Xnor,
    OP_Not, OP_Nand, OP_Nor, OP_Imply,
};
// clang-format on

typedef struct {
    const char *name;
    usize index;
} var;

typedef struct {
    var *items;
    usize size;
    usize capacity;
} vars;

typedef struct {
    u8 *items;
    usize size;
    usize capacity;

    vars vars;
} chunk;

#endif // CHUNK_H
