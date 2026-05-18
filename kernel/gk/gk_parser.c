//ember2819
#include <gk/gk.h>

// helpers
static void strcpy_s(char* d, const char* s) {
    int i = 0; while (s[i]) { d[i] = s[i]; i++; } d[i] = 0;
}

static int new_node(GkState* s, NodeType t, int line) {
    if (s->node_count >= GK_MAX_NODES) {
        strcpy_s(s->error, "too many AST nodes");
        return -1;
    }
    int idx = s->node_count++;
    Node* n  = &s->nodes[idx];
    n->type       = t;
    n->line       = line;
    n->int_val    = 0;
    n->n_children = 0;
    n->str_val[0] = 0;
    for (int i = 0; i < 8; i++) n->child[i] = -1;
    return idx;
}

static Token* peek(GkState* s) {
    return &s->tokens[s->tok_pos];
}
static Token* advance(GkState* s) {
    Token* t = &s->tokens[s->tok_pos];
    if (t->type != TOK_EOF) s->tok_pos++;
    return t;
}
static bool check(GkState* s, TokenType t) {
    return peek(s)->type == t;
}
static bool match(GkState* s, TokenType t) {
    if (check(s, t)) { advance(s); return true; }
    return false;
}
static bool expect(GkState* s, TokenType t, const char* msg) {
    if (match(s, t)) return true;
    strcpy_s(s->error, msg);
    return false;
}

// Skip blank newlines between statements
static void skip_newlines(GkState* s) {
    while (check(s, TOK_NEWLINE)) advance(s);
}

static int parse_expr(GkState* s);
static int parse_stmt(GkState* s);
static int parse_block(GkState* s);

static int parse_primary(GkState* s) {
    Token* t = peek(s);
    int line  = t->line;

    if (t->type == TOK_INT) {
        advance(s);
        int n = new_node(s, NODE_INT, line);
        if (n < 0) return -1;
        s->nodes[n].int_val = t->int_val;
        return n;
    }
    if (t->type == TOK_STR) {
        advance(s);
        int n = new_node(s, NODE_STR, line);
        if (n < 0) return -1;
        strcpy_s(s->nodes[n].str_val, t->str_val);
        return n;
    }
    if (t->type == TOK_TRUE || t->type == TOK_FALSE) {
        advance(s);
        int n = new_node(s, NODE_BOOL, line);
        if (n < 0) return -1;
        s->nodes[n].int_val = (t->type == TOK_TRUE) ? 1 : 0;
        return n;
    }
    if (t->type == TOK_IDENT) {
        advance(s);
        if (check(s, TOK_LPAREN)) {
            advance(s);

            NodeType nt = NODE_CALL;
            // Check for print builtin
            if (t->str_val[0] == 'p' && t->str_val[1] == 'r' &&
                t->str_val[2] == 'i' && t->str_val[3] == 'n' &&
                t->str_val[4] == 't' && t->str_val[5] == 0) {
                nt = NODE_PRINT;
            }
            // Check for range builtin
            if (t->str_val[0] == 'r' && t->str_val[1] == 'a' &&
                t->str_val[2] == 'n' && t->str_val[3] == 'g' &&
                t->str_val[4] == 'e' && t->str_val[5] == 0) {
                nt = NODE_RANGE;
            }
            // Check for input() builtin
            if (t->str_val[0] == 'i' && t->str_val[1] == 'n' &&
                t->str_val[2] == 'p' && t->str_val[3] == 'u' &&
                t->str_val[4] == 't' && t->str_val[5] == 0) {
                nt = NODE_INPUT;
            }
            // Check for int() builtin
            if (t->str_val[0] == 'i' && t->str_val[1] == 'n' &&
                t->str_val[2] == 't' && t->str_val[3] == 0) {
                nt = NODE_INT_CAST;
            }

            int n = new_node(s, nt, line);
            if (n < 0) return -1;
            strcpy_s(s->nodes[n].str_val, t->str_val);

            while (!check(s, TOK_RPAREN) && !check(s, TOK_EOF)) {
                int arg = parse_expr(s);
                if (arg < 0) return -1;
                if (s->nodes[n].n_children >= GK_MAX_CHILDREN) {
                    strcpy_s(s->error, "too many arguments");
                    return -1;
                }
                s->nodes[n].child[s->nodes[n].n_children++] = arg;
                if (!match(s, TOK_COMMA)) break;
            }
            if (!expect(s, TOK_RPAREN, "expected ')' after arguments")) return -1;
            return n;
        }
        int n = new_node(s, NODE_VAR, line);
        if (n < 0) return -1;
        strcpy_s(s->nodes[n].str_val, t->str_val);
        return n;
    }
    if (t->type == TOK_LPAREN) {
        advance(s);
        int n = parse_expr(s);
        if (n < 0) return -1;
        if (!expect(s, TOK_RPAREN, "expected ')'")) return -1;
        return n;
    }

    strcpy_s(s->error, "unexpected token in expression");
    return -1;
}

static int parse_unary(GkState* s) {
    int line = peek(s)->line;
    if (check(s, TOK_MINUS)) {
        advance(s);
        int operand = parse_unary(s);
        if (operand < 0) return -1;
        int n = new_node(s, NODE_UNOP, line);
        if (n < 0) return -1;
        s->nodes[n].op = TOK_MINUS;
        s->nodes[n].child[0] = operand;
        s->nodes[n].n_children = 1;
        return n;
    }
    if (check(s, TOK_NOT)) {
        advance(s);
        int operand = parse_unary(s);
        if (operand < 0) return -1;
        int n = new_node(s, NODE_UNOP, line);
        if (n < 0) return -1;
        s->nodes[n].op = TOK_NOT;
        s->nodes[n].child[0] = operand;
        s->nodes[n].n_children = 1;
        return n;
    }
    return parse_primary(s);
}

static int parse_mul(GkState* s) {
    int left = parse_unary(s);
    if (left < 0) return -1;
    while (check(s, TOK_STAR) || check(s, TOK_SLASH) || check(s, TOK_PERCENT)) {
        TokenType op = peek(s)->type;
        int line = peek(s)->line;
        advance(s);
        int right = parse_unary(s);
        if (right < 0) return -1;
        int n = new_node(s, NODE_BINOP, line);
        if (n < 0) return -1;
        s->nodes[n].op = op;
        s->nodes[n].child[0] = left;
        s->nodes[n].child[1] = right;
        s->nodes[n].n_children = 2;
        left = n;
    }
    return left;
}

static int parse_add(GkState* s) {
    int left = parse_mul(s);
    if (left < 0) return -1;
    while (check(s, TOK_PLUS) || check(s, TOK_MINUS)) {
        TokenType op = peek(s)->type;
        int line = peek(s)->line;
        advance(s);
        int right = parse_mul(s);
        if (right < 0) return -1;
        int n = new_node(s, NODE_BINOP, line);
        if (n < 0) return -1;
        s->nodes[n].op = op;
        s->nodes[n].child[0] = left;
        s->nodes[n].child[1] = right;
        s->nodes[n].n_children = 2;
        left = n;
    }
    return left;
}

static int parse_cmp(GkState* s) {
    int left = parse_add(s);
    if (left < 0) return -1;
    while (check(s, TOK_EQ)  || check(s, TOK_NEQ) ||
           check(s, TOK_LT)  || check(s, TOK_LE)  ||
           check(s, TOK_GT)  || check(s, TOK_GE)) {
        TokenType op = peek(s)->type;
        int line = peek(s)->line;
        advance(s);
        int right = parse_add(s);
        if (right < 0) return -1;
        int n = new_node(s, NODE_BINOP, line);
        if (n < 0) return -1;
        s->nodes[n].op = op;
        s->nodes[n].child[0] = left;
        s->nodes[n].child[1] = right;
        s->nodes[n].n_children = 2;
        left = n;
    }
    return left;
}

static int parse_and(GkState* s) {
    int left = parse_cmp(s);
    if (left < 0) return -1;
    while (check(s, TOK_AND)) {
        int line = peek(s)->line;
        advance(s);
        int right = parse_cmp(s);
        if (right < 0) return -1;
        int n = new_node(s, NODE_BINOP, line);
        if (n < 0) return -1;
        s->nodes[n].op = TOK_AND;
        s->nodes[n].child[0] = left;
        s->nodes[n].child[1] = right;
        s->nodes[n].n_children = 2;
        left = n;
    }
    return left;
}

static int parse_or(GkState* s) {
    int left = parse_and(s);
    if (left < 0) return -1;
    while (check(s, TOK_OR)) {
        int line = peek(s)->line;
        advance(s);
        int right = parse_and(s);
        if (right < 0) return -1;
        int n = new_node(s, NODE_BINOP, line);
        if (n < 0) return -1;
        s->nodes[n].op = TOK_OR;
        s->nodes[n].child[0] = left;
        s->nodes[n].child[1] = right;
        s->nodes[n].n_children = 2;
        left = n;
    }
    return left;
}

static int parse_expr(GkState* s) {
    return parse_or(s);
}

static int parse_block(GkState* s) {
    int line = peek(s)->line;
    if (!expect(s, TOK_INDENT, "expected indented block")) return -1;

    int block = new_node(s, NODE_BLOCK, line);
    if (block < 0) return -1;

    skip_newlines(s);
    while (!check(s, TOK_DEDENT) && !check(s, TOK_EOF)) {
        int st = parse_stmt(s);
        if (st < 0) return -1;
        if (s->nodes[block].n_children >= GK_MAX_CHILDREN) {
            strcpy_s(s->error, "block too large");
            return -1;
        }
        s->nodes[block].child[s->nodes[block].n_children++] = st;
        skip_newlines(s);
    }
    match(s, TOK_DEDENT);
    return block;
}

static int parse_stmt(GkState* s) {
    Token* t  = peek(s);
    int line  = t->line;

    if (t->type == TOK_RETURN) {
        advance(s);
        int n = new_node(s, NODE_RETURN, line);
        if (n < 0) return -1;
        if (!check(s, TOK_NEWLINE) && !check(s, TOK_EOF)) {
            int expr = parse_expr(s);
            if (expr < 0) return -1;
            s->nodes[n].child[0] = expr;
            s->nodes[n].n_children = 1;
        }
        match(s, TOK_NEWLINE);
        return n;
    }

    if (t->type == TOK_BREAK) {
        advance(s);
        match(s, TOK_NEWLINE);
        return new_node(s, NODE_BREAK, line);
    }

    if (t->type == TOK_CONTINUE) {
        advance(s);
        match(s, TOK_NEWLINE);
        return new_node(s, NODE_CONTINUE, line);
    }

    if (t->type == TOK_DEF) {
        advance(s);
        Token* name = peek(s);
        if (name->type != TOK_IDENT) {
            strcpy_s(s->error, "expected function name after 'def'");
            return -1;
        }
        advance(s);

        int n = new_node(s, NODE_DEF, line);
        if (n < 0) return -1;
        strcpy_s(s->nodes[n].str_val, name->str_val);

        if (!expect(s, TOK_LPAREN, "expected '(' after function name")) return -1;
        while (!check(s, TOK_RPAREN) && !check(s, TOK_EOF)) {
            Token* param = peek(s);
            if (param->type != TOK_IDENT) {
                strcpy_s(s->error, "expected parameter name");
                return -1;
            }
            advance(s);
            int pn = new_node(s, NODE_VAR, param->line);
            if (pn < 0) return -1;
            strcpy_s(s->nodes[pn].str_val, param->str_val);
            s->nodes[n].child[s->nodes[n].n_children++] = pn;
            if (!match(s, TOK_COMMA)) break;
        }
        if (!expect(s, TOK_RPAREN, "expected ')' after parameters")) return -1;
        if (!expect(s, TOK_COLON,  "expected ':' after def")) return -1;
        match(s, TOK_NEWLINE);

        int body = parse_block(s);
        if (body < 0) return -1;
        s->nodes[n].child[s->nodes[n].n_children++] = body;
        return n;
    }

    if (t->type == TOK_IF || t->type == TOK_ELIF) {
        advance(s);
        int n = new_node(s, NODE_IF, line);
        if (n < 0) return -1;

        int cond = parse_expr(s);
        if (cond < 0) return -1;
        if (!expect(s, TOK_COLON, "expected ':' after if condition")) return -1;
        match(s, TOK_NEWLINE);
        int body = parse_block(s);
        if (body < 0) return -1;

        s->nodes[n].child[0] = cond;
        s->nodes[n].child[1] = body;
        s->nodes[n].n_children = 2;

        // optional elif / else
        skip_newlines(s);
        if (check(s, TOK_ELIF)) {
            int elif_node = parse_stmt(s);
            if (elif_node < 0) return -1;
            s->nodes[n].child[2] = elif_node;
            s->nodes[n].n_children = 3;
        } else if (check(s, TOK_ELSE)) {
            advance(s);
            if (!expect(s, TOK_COLON, "expected ':' after else")) return -1;
            match(s, TOK_NEWLINE);
            int else_body = parse_block(s);
            if (else_body < 0) return -1;
            s->nodes[n].child[2] = else_body;
            s->nodes[n].n_children = 3;
        }
        return n;
    }

    if (t->type == TOK_WHILE) {
        advance(s);
        int n = new_node(s, NODE_WHILE, line);
        if (n < 0) return -1;

        int cond = parse_expr(s);
        if (cond < 0) return -1;
        if (!expect(s, TOK_COLON, "expected ':' after while condition")) return -1;
        match(s, TOK_NEWLINE);
        int body = parse_block(s);
        if (body < 0) return -1;

        s->nodes[n].child[0] = cond;
        s->nodes[n].child[1] = body;
        s->nodes[n].n_children = 2;
        return n;
    }

    if (t->type == TOK_FOR) {
        advance(s);
        Token* var = peek(s);
        if (var->type != TOK_IDENT) {
            strcpy_s(s->error, "expected variable name after 'for'");
            return -1;
        }
        advance(s);
        if (!expect(s, TOK_IN, "expected 'in' after for variable")) return -1;

        int n = new_node(s, NODE_FOR, line);
        if (n < 0) return -1;
        strcpy_s(s->nodes[n].str_val, var->str_val);

        int iter = parse_expr(s);
        if (iter < 0) return -1;
        if (!expect(s, TOK_COLON, "expected ':' after for iterable")) return -1;
        match(s, TOK_NEWLINE);
        int body = parse_block(s);
        if (body < 0) return -1;

        s->nodes[n].child[0] = iter;
        s->nodes[n].child[1] = body;
        s->nodes[n].n_children = 2;
        return n;
    }

    if (t->type == TOK_IDENT && s->tok_pos + 1 < s->tok_count &&
        s->tokens[s->tok_pos + 1].type == TOK_ASSIGN) {
        char name[64];
        strcpy_s(name, t->str_val);
        advance(s);
        advance(s);

        int n = new_node(s, NODE_ASSIGN, line);
        if (n < 0) return -1;
        strcpy_s(s->nodes[n].str_val, name);

        int expr = parse_expr(s);
        if (expr < 0) return -1;
        s->nodes[n].child[0] = expr;
        s->nodes[n].n_children = 1;
        match(s, TOK_NEWLINE);
        return n;
    }

    int expr = parse_expr(s);
    if (expr < 0) return -1;
    match(s, TOK_NEWLINE);
    return expr;
}

int gk_parse(GkState* s) {
    s->node_count = 0;
    s->tok_pos    = 0;

    int root = new_node(s, NODE_BLOCK, 0);
    if (root < 0) return -1;

    skip_newlines(s);
    while (!check(s, TOK_EOF)) {
        int st = parse_stmt(s);
        if (st < 0) return -1;
        if (s->nodes[root].n_children >= GK_MAX_CHILDREN) {
            strcpy_s(s->error, "too many top-level statements");
            return -1;
        }
        s->nodes[root].child[s->nodes[root].n_children++] = st;
        skip_newlines(s);
    }
    return root;
}
