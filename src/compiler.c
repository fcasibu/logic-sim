global parser Parser;
global chunk *CompilingChunk;

#define MAX_VARS 10

#define PARSE_FN(name) void name(void)
internal PARSE_FN(Grouping);
internal PARSE_FN(Binary);
internal PARSE_FN(Unary);
internal PARSE_FN(Var);

// clang-format off
global_const parse_rule RULES[] = {
    [TokenKind_LeftParen]  = { Grouping, NULL,   Prec_None  },
    [TokenKind_RightParen] = { NULL,     NULL,   Prec_None  },
    [TokenKind_Identifier] = { Var,      NULL,   Prec_None  },
    [TokenKind_Xor]        = { NULL,     Binary, Prec_Xor   },
    [TokenKind_And]        = { NULL,     Binary, Prec_And   },
    [TokenKind_Nand]       = { NULL,     Binary, Prec_And   },
    [TokenKind_Or]         = { NULL,     Binary, Prec_Or    },
    [TokenKind_Nor]        = { NULL,     Binary, Prec_Or    },
    [TokenKind_Xnor]       = { NULL,     Binary, Prec_Xor   },
    [TokenKind_Imply]      = { NULL,     Binary, Prec_Imply },
    [TokenKind_Not]        = { Unary,    NULL,   Prec_Not   },
    [TokenKind_Error]      = { NULL,     NULL,   Prec_None  },
    [TokenKind_Eof]        = { NULL,     NULL,   Prec_None  },
};
// clang-format on

internal void
InitializeParser(memory_arena *arena)
{
    Assert(arena);

    Parser.arena = arena;
    Parser.had_error = false;
}

internal inline void
EmitByte(u8 byte)
{
    WriteChunk(Parser.arena, CompilingChunk, byte);
}

internal inline void
EmitVar(const char *name, usize idx)
{
    WriteVar(Parser.arena, CompilingChunk, name, idx);
}

internal inline const parse_rule *
GetRule(token_kind kind)
{
    return &RULES[kind];
}

internal void
AdvanceParser(void)
{
    Parser.previous = Parser.current;

    for (;;) {
        Parser.current = ScanToken();

        if (Parser.current.kind != TokenKind_Error)
            break;

        // TODO(fcasibu): Reporting
        Parser.had_error = true;
    }
}

internal inline void
ConsumeParser(token_kind kind)
{
    if (Parser.current.kind == kind) {
        AdvanceParser();
        return;
    }

    // TODO(fcasibu): Reporting
    Parser.had_error = true;
}

internal void
ParsePrecedence(token_precedence precedence)
{
    AdvanceParser();
    parse_fn PrefixRule = GetRule(Parser.previous.kind)->prefix;

    if (!PrefixRule) {
        // TODO(fcasibu): Reporting
        Parser.had_error = true;
        return;
    }

    PrefixRule();

    while (precedence <= GetRule(Parser.current.kind)->precedence) {
        AdvanceParser();

        parse_fn InfixRule = GetRule(Parser.previous.kind)->infix;
        Assert(InfixRule);
        InfixRule();
    }
}

internal inline void
Expression(void)
{
    ParsePrecedence(Prec_Imply);
}

internal inline PARSE_FN(Unary)
{
    token tok = Parser.previous;
    ParsePrecedence(GetRule(tok.kind)->precedence + 1);

    switch (tok.kind) {
        case TokenKind_Not: {
            EmitByte(OP_Not);
        } break;

            INVALID_DEFAULT_CASE;
    }
}

internal inline PARSE_FN(Binary)
{
    token_kind kind = Parser.previous.kind;
    ParsePrecedence(GetRule(kind)->precedence + 1);

    switch (kind) {
        case TokenKind_And: {
            EmitByte(OP_And);
        } break;

        case TokenKind_Nand: {
            EmitByte(OP_Nand);
        } break;

        case TokenKind_Xnor: {
            EmitByte(OP_Xnor);
        } break;

        case TokenKind_Xor: {
            EmitByte(OP_Xor);
        } break;

        case TokenKind_Or: {
            EmitByte(OP_Or);
        } break;

        case TokenKind_Nor: {
            EmitByte(OP_Nor);
        } break;

        case TokenKind_Imply: {
            EmitByte(OP_Imply);
        } break;

            INVALID_DEFAULT_CASE;
    }
}

internal inline PARSE_FN(Var)
{
    token tok = Parser.previous;

    char buf[tok.length + 1];
    strncpy(buf, tok.lexeme_start, tok.length);
    buf[tok.length] = '\0';

    const char *interned_string = InternString(buf);
    Assert(interned_string);
    i64 idx = GetInternedStringIdx(interned_string);
    Assert(idx >= 0);

    if (idx + 1 > MAX_VARS) {
        Parser.had_error = true;
        return;
    }

    EmitVar(interned_string, idx);
}

internal inline PARSE_FN(Grouping)
{
    Expression();
    ConsumeParser(TokenKind_RightParen);
}

internal b32
Parse(memory_arena *arena, chunk *c, const char *source)
{
    Assert(arena);
    Assert(c);
    Assert(source);

    InitializeLexer(source);
    InitializeParser(arena);
    CompilingChunk = c;

    AdvanceParser();
    Expression();

    return !Parser.had_error;
}
