global parser Parser;

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
    [TokenKind_Xnor]       = { NULL,     Binary, Prec_Xnor  },
    [TokenKind_Imply]      = { NULL,     Binary, Prec_Imply },
    [TokenKind_Not]        = { Unary,    NULL,   Prec_Not   },
    [TokenKind_Error]      = { NULL,     NULL,   Prec_None  },
    [TokenKind_Eof]        = { NULL,     NULL,   Prec_None  },
};
// clang-format on

internal void
InitializeParser(memory_arena *arena, ast_nodes *nodes)
{
    Assert(arena);
    Assert(nodes);

    Parser.nodes = nodes;
    Parser.arena = arena;
    Parser.had_error = false;
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
    }
}

internal inline void
ConsumeParser(token_kind kind)
{
    if (Parser.current.kind == kind) {
        AdvanceParser();
        return;
    }

    Todo("Reporting");
}

internal void
ParsePrecedence(token_precedence precedence)
{
    AdvanceParser();
    parse_fn PrefixRule = GetRule(Parser.previous.kind)->prefix;

    if (!PrefixRule) {
        Todo("Reporting");
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
    ParsePrecedence(Prec_Xnor);
}

internal inline PARSE_FN(Unary)
{
    token tok = Parser.previous;
    ParsePrecedence(GetRule(tok.kind)->precedence + 1);

    switch (tok.kind) {
        case TokenKind_Not: {
            ast_node child = GetLastNode(Parser.nodes);
            node_value as = {
                .unary = { .op = tok.kind, .child = child.id, },
            };
            MakeNode(Parser.arena, Parser.nodes, Node_Not, as, tok.col, child.end);
        } break;

            INVALID_DEFAULT_CASE;
    }
}

internal inline PARSE_FN(Binary)
{
    ast_node left = GetLastNode(Parser.nodes);

    token_kind kind = Parser.previous.kind;
    ParsePrecedence(GetRule(kind)->precedence + 1);

    ast_node right = GetLastNode(Parser.nodes);
    node_value as = {
        .binary = { .op = kind, .left = left.id, .right = right.id, },
    };

    switch (kind) {
        case TokenKind_And: {
            MakeNode(Parser.arena, Parser.nodes, Node_And, as, left.start, right.end);
        } break;

        case TokenKind_Nand: {
            MakeNode(Parser.arena, Parser.nodes, Node_Nand, as, left.start, right.end);
        } break;

        case TokenKind_Xnor: {
            MakeNode(Parser.arena, Parser.nodes, Node_Xnor, as, left.start, right.end);
        } break;

        case TokenKind_Xor: {
            MakeNode(Parser.arena, Parser.nodes, Node_Xor, as, left.start, right.end);
        } break;

        case TokenKind_Or: {
            MakeNode(Parser.arena, Parser.nodes, Node_Or, as, left.start, right.end);
        } break;

        case TokenKind_Nor: {
            MakeNode(Parser.arena, Parser.nodes, Node_Nor, as, left.start, right.end);
        } break;

        case TokenKind_Imply: {
            MakeNode(Parser.arena, Parser.nodes, Node_Imply, as, left.start, right.end);
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

    node_value as = {
        .var = InternString(buf),
    };

    MakeNode(Parser.arena, Parser.nodes, Node_Var, as, tok.col, tok.col + tok.length - 1);
}

internal inline PARSE_FN(Grouping)
{
    Expression();
    ConsumeParser(TokenKind_RightParen);
}

internal b32
Parse(memory_arena *arena, ast_nodes *nodes, const char *source)
{
    InitializeLexer(source);
    InitializeParser(arena, nodes);

    AdvanceParser();
    Expression();

    return !Parser.had_error;
}
