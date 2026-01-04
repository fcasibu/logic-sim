#ifndef PARSER_H
#define PARSER_H

// clang-format off
typedef Enum(u8, token_precedence){
    Prec_None,
    Prec_Xnor,
    Prec_Imply,
    Prec_Or,
    Prec_Xor,
    Prec_And,
    Prec_Not,
};
// clang-format on

typedef void (*parse_fn)(void);

typedef struct {
    parse_fn prefix;
    parse_fn infix;
    token_precedence precedence;
} parse_rule;

typedef struct {
    ast_nodes *nodes;
    memory_arena *arena;

    token previous;
    token current;

    b32 had_error;
} parser;

#endif // PARSER_H
