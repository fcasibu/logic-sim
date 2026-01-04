internal void
InitializeNodes(memory_arena *arena, ast_nodes *nodes, usize initial_cap)
{
    Assert(nodes);

    nodes->items = PushArray(arena, initial_cap, typeof(*nodes->items));
    nodes->size = 0;
    nodes->capacity = initial_cap;

    Assert(nodes->items);
}

internal inline ast_node *
GetNode(ast_nodes *nodes, node_id id)
{
    if (id == NULL_NODE)
        return NULL;

    return &nodes->items[id - 1];
}

internal inline ast_node
GetLastNode(ast_nodes *nodes)
{
    return nodes->items[nodes->size - 1];
}

internal inline void
MakeNode(memory_arena *arena,
         ast_nodes *nodes,
         node_kind kind,
         node_value as,
         usize start,
         usize end)
{
    ast_node result = { 0 };
    result.kind = kind;
    result.id = nodes->size + 1;
    result.start = start;
    result.end = end;
    result.as = as;

    if (nodes->size >= nodes->capacity)
        GrowArray(arena, nodes);

    Assert(nodes->size < nodes->capacity);
    nodes->items[nodes->size++] = result;
}

internal const char *
NodeKindToString(node_kind kind)
{
    switch (kind) {
        case Node_Var:
            return "Var";
        case Node_Xor:
            return "Xor";
        case Node_Xnor:
            return "Xnor";
        case Node_Or:
            return "Or";
        case Node_And:
            return "And";
        case Node_Not:
            return "Not";
        case Node_Nand:
            return "Nand";
        case Node_Nor:
            return "Nor";
        case Node_Imply:
            return "Imply";
        default:
            return "Unknown";
    }
}

internal void
PrintAstNode(ast_nodes *nodes, node_id id, usize indent_level)
{
    ast_node *node = GetNode(nodes, id);
    if (!node) {
        printf("null");
        return;
    }

#define Indent()                             \
    for (usize i = 0; i < indent_level; ++i) \
    printf("  ")

    printf("{\n");
    indent_level++;

    Indent();
    printf("\"id\": %zu,\n", node->id);
    Indent();
    printf("\"kind\": \"%s\",\n", NodeKindToString(node->kind));
    Indent();
    printf("\"range\": [%zu, %zu],\n", node->start, node->end);

    switch (node->kind) {
        case Node_Var: {
            Indent();
            printf("\"value\": \"%s\"\n", node->as.var);
        } break;

        case Node_Not: {
            Indent();
            printf("\"op\": %d,\n", node->as.unary.op);
            Indent();
            printf("\"child\": ");
            PrintAstNode(nodes, node->as.unary.child, indent_level);
            printf("\n");
        } break;

        case Node_Xor:
        case Node_Xnor:
        case Node_Or:
        case Node_And:
        case Node_Nor:
        case Node_Imply:
        case Node_Nand: {
            Indent();
            printf("\"op\": %d,\n", node->as.binary.op);
            Indent();
            printf("\"left\": ");
            PrintAstNode(nodes, node->as.binary.left, indent_level);
            printf(",\n");
            Indent();
            printf("\"right\": ");
            PrintAstNode(nodes, node->as.binary.right, indent_level);
            printf("\n");
        } break;
    }

    indent_level--;
    Indent();
    printf("}");

#undef INDENT
}

internal void
PrintAst(ast_nodes *nodes)
{
    if (!nodes->size) {
        printf("{ \"error\": \"empty tree\" }\n");
        return;
    }

    node_id root_id = GetLastNode(nodes).id;
    PrintAstNode(nodes, root_id, 0);
    printf("\n");
}
