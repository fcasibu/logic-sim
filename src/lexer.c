#include <ctype.h>

global lexer Lexer;

global_const struct {
    const char *identifier;
    token_kind kind;
} RESERVED_WORDS_TABLE[] = {
    // clang-format off
    {"XOR",   TokenKind_Xor   },
    {"XNOR",  TokenKind_Xnor  },
    {"OR",    TokenKind_Or    },
    {"NOR",   TokenKind_Nor   },
    {"AND",   TokenKind_And   },
    {"NAND",  TokenKind_Nand  },
    {"NOT",   TokenKind_Not   },
    {"IMPLY", TokenKind_Imply },
    // clang-format on
};

internal void
InitializeLexer(const char *source)
{
    Lexer.lexeme_start = source;
    Lexer.current_char = source;
    Lexer.col = 1;
}

internal token_kind
LookupReservedWord(const char *s)
{
    usize n = ArrayCount(RESERVED_WORDS_TABLE);

    for (usize i = 0; i < n; ++i) {
        if (strcmp(RESERVED_WORDS_TABLE[i].identifier, s) == 0)
            return RESERVED_WORDS_TABLE[i].kind;
    }

    return TokenKind_Identifier;
}

internal inline token
MakeToken(token_kind kind)
{
    token result = { 0 };
    result.kind = kind;
    result.lexeme_start = Lexer.lexeme_start;
    result.length = Lexer.current_char - Lexer.lexeme_start;
    result.col = Lexer.col - result.length;

    return result;
}

internal inline char
PeekLexer(void)
{
    return *Lexer.current_char;
}

internal inline char
AdvanceLexer(void)
{
    char c = *Lexer.current_char++;
    Lexer.col += 1;

    return c;
}

internal token
LexerIdentifier(void)
{
    while (isalpha(PeekLexer()) && PeekLexer() != '\0')
        AdvanceLexer();

    usize length = Lexer.current_char - Lexer.lexeme_start;
    char buf[length + 1];
    strncpy(buf, Lexer.lexeme_start, length);
    buf[length] = '\0';

    return MakeToken(LookupReservedWord(buf));
}

internal token
ScanToken(void)
{
    for (;;) {
        char c = PeekLexer();
        if (isspace(c)) {
            AdvanceLexer();
        } else {
            break;
        }
    }

    Lexer.lexeme_start = Lexer.current_char;

    if (PeekLexer() == '\0')
        return MakeToken(TokenKind_Eof);

    char ch = AdvanceLexer();

    switch (ch) {
        case '\0':
            return MakeToken(TokenKind_Eof);
        case '(':
            return MakeToken(TokenKind_LeftParen);
        case ')':
            return MakeToken(TokenKind_RightParen);
        default: {
            if (isalpha(ch))
                return LexerIdentifier();

            Todo("Reporting");
        };
    }
}
