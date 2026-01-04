#ifndef LEXER_H
#define LEXER_H

// clang-format off
typedef Enum(u8, token_kind){
    TokenKind_LeftParen, TokenKind_RightParen, TokenKind_Identifier,

    TokenKind_Xor, TokenKind_And, TokenKind_Or, TokenKind_Not, TokenKind_Nand,
    TokenKind_Xnor, TokenKind_Nor, TokenKind_Imply,

    TokenKind_Error,TokenKind_Eof,
};
// clang-format on

typedef struct {
    token_kind kind;

    const char *lexeme_start;
    usize length;

    usize col;
} token;

typedef struct {
    const char *lexeme_start;
    const char *current_char;

    usize col;
} lexer;

#endif // LEXER_H
