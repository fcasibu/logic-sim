#ifndef NODE_H
#define NODE_H

typedef usize node_id;

#define NULL_NODE 0

// clang-format off
typedef Enum(u8, node_kind) {
    Node_Var,

    Node_Xor, Node_Xnor, Node_Or, Node_And, Node_Not, 
    Node_Nand, Node_Nor, Node_Imply,
};
// clang-format on

typedef union {
    const char *var;

    struct {
        node_id left;
        node_id right;
        token_kind op;
    } binary;

    struct {
        node_id child;
        token_kind op;
    } unary;
} node_value;

typedef struct {
    node_kind kind;
    node_value as;

    usize start;
    usize end;

    node_id id;
} ast_node;

typedef struct {
    ast_node *items;
    usize size;
    usize capacity;
} ast_nodes;

#endif // NODE_H
