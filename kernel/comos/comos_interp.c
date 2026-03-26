#include "comos.h"

// helpers

static void strcpy_s(char* d, const char* s) {
    int i = 0; while (s[i]) { d[i] = s[i]; i++; } d[i] = 0;
}
static bool streq(const char* a, const char* b) {
    int i = 0;
    while (a[i] && b[i]) { if (a[i] != b[i]) return false; i++; }
    return a[i] == b[i];
}
static int strlen_s(const char* s) {
    int n = 0; while (s[n]) n++; return n;
}

static void strcat_s(char* dst, const char* src) {
    int d = strlen_s(dst);
    int i = 0;
    while (src[i] && d + i < COMOS_MAX_STR - 1) {
        dst[d + i] = src[i];
        i++;
    }
    dst[d + i] = 0;
}

// Convert integer to decimal string.
void int_to_str(int v, char* buf) {
    if (v == 0) { buf[0] = '0'; buf[1] = 0; return; }
    char tmp[32];
    int neg = 0, i = 0;
    if (v < 0) { neg = 1; v = -v; }
    while (v > 0) { tmp[i++] = '0' + (v % 10); v /= 10; }
    if (neg) tmp[i++] = '-';
    for (int j = 0; j < i; j++) buf[j] = tmp[i - 1 - j];
    buf[i] = 0;
}

static Value val_int(int v)         { Value r; r.type=VAL_INT;  r.int_val=v; r.bool_val=0; r.str_val[0]=0; return r; }
static Value val_bool(bool v)       { Value r; r.type=VAL_BOOL; r.bool_val=v; r.int_val=0; r.str_val[0]=0; return r; }
static Value val_none()             { Value r; r.type=VAL_NONE; r.int_val=0; r.bool_val=0; r.str_val[0]=0; return r; }
static Value val_str(const char* v) {
    Value r; r.type=VAL_STR; r.int_val=0; r.bool_val=0;
    strcpy_s(r.str_val, v); return r;
}

static bool is_truthy(Value v) {
    if (v.type == VAL_BOOL) return v.bool_val;
    if (v.type == VAL_INT)  return v.int_val != 0;
    if (v.type == VAL_STR)  return v.str_val[0] != 0;
    return false;
}

// Convert any value to a string
static void val_to_str(Value v, char* buf) {
    if (v.type == VAL_INT)  { int_to_str(v.int_val, buf); return; }
    if (v.type == VAL_BOOL) { strcpy_s(buf, v.bool_val ? "True" : "False"); return; }
    if (v.type == VAL_STR)  { strcpy_s(buf, v.str_val); return; }
    strcpy_s(buf, "None");
}

static Var* find_var(ComosState* s, const char* name) {
    for (int i = s->var_count - 1; i >= 0; i--) {
        if (streq(s->vars[i].name, name)) return &s->vars[i];
    }
    return 0;
}

static bool set_var(ComosState* s, const char* name, Value val) {
    for (int i = s->var_count - 1; i >= 0; i--) {
        if (s->vars[i].scope < s->call_depth) break;
        if (streq(s->vars[i].name, name)) {
            s->vars[i].val = val;
            return true;
        }
    }

    if (s->var_count >= COMOS_MAX_VARS) {
        strcpy_s(s->error, "too many variables");
        return false;
    }
    strcpy_s(s->vars[s->var_count].name, name);
    s->vars[s->var_count].val   = val;
    s->vars[s->var_count].scope = s->call_depth;
    s->var_count++;
    return true;
}

static void pop_scope(ComosState* s) {
    while (s->var_count > 0 &&
           s->vars[s->var_count - 1].scope >= s->call_depth) {
        s->var_count--;
    }
}

// Find a function by name
static FuncDef* find_func(ComosState* s, const char* name) {
    for (int i = 0; i < s->func_count; i++) {
        if (streq(s->funcs[i].name, name)) return &s->funcs[i];
    }
    return 0;
}

static Value eval(ComosState* s, int node_idx);

static Value eval(ComosState* s, int node_idx) {
    if (node_idx < 0 || s->error[0]) return val_none();

    Node* n = &s->nodes[node_idx];

    switch (n->type) {

    case NODE_INT:  return val_int(n->int_val);
    case NODE_STR:  return val_str(n->str_val);
    case NODE_BOOL: return val_bool(n->int_val != 0);

    case NODE_VAR: {
        Var* v = find_var(s, n->str_val);
        if (!v) {
            strcpy_s(s->error, "undefined variable: ");
            strcat_s(s->error, n->str_val);
            return val_none();
        }
        return v->val;
    }

    case NODE_ASSIGN: {
        Value val = eval(s, n->child[0]);
        if (s->error[0]) return val_none();
        if (!set_var(s, n->str_val, val)) return val_none();
        return val;
    }

    case NODE_UNOP: {
        Value operand = eval(s, n->child[0]);
        if (s->error[0]) return val_none();
        if (n->op == TOK_MINUS) {
            if (operand.type != VAL_INT) {
                strcpy_s(s->error, "unary minus requires integer");
                return val_none();
            }
            return val_int(-operand.int_val);
        }
        if (n->op == TOK_NOT) return val_bool(!is_truthy(operand));
        return val_none();
    }

    case NODE_BINOP: {
        if (n->op == TOK_AND) {
            Value l = eval(s, n->child[0]);
            if (!is_truthy(l)) return val_bool(false);
            return val_bool(is_truthy(eval(s, n->child[1])));
        }
        if (n->op == TOK_OR) {
            Value l = eval(s, n->child[0]);
            if (is_truthy(l)) return val_bool(true);
            return val_bool(is_truthy(eval(s, n->child[1])));
        }

        Value l = eval(s, n->child[0]);
        if (s->error[0]) return val_none();
        Value r = eval(s, n->child[1]);
        if (s->error[0]) return val_none();

        if (n->op == TOK_PLUS && l.type == VAL_STR) {
            char buf[COMOS_MAX_STR];
            strcpy_s(buf, l.str_val);
            char rbuf[COMOS_MAX_STR];
            val_to_str(r, rbuf);
            strcat_s(buf, rbuf);
            return val_str(buf);
        }

        if (n->op == TOK_EQ) {
            if (l.type == VAL_INT  && r.type == VAL_INT)  return val_bool(l.int_val  == r.int_val);
            if (l.type == VAL_BOOL && r.type == VAL_BOOL) return val_bool(l.bool_val == r.bool_val);
            if (l.type == VAL_STR  && r.type == VAL_STR)  return val_bool(streq(l.str_val, r.str_val));
            return val_bool(false);
        }
        if (n->op == TOK_NEQ) {
            if (l.type == VAL_INT  && r.type == VAL_INT)  return val_bool(l.int_val  != r.int_val);
            if (l.type == VAL_BOOL && r.type == VAL_BOOL) return val_bool(l.bool_val != r.bool_val);
            if (l.type == VAL_STR  && r.type == VAL_STR)  return val_bool(!streq(l.str_val, r.str_val));
            return val_bool(true);
        }

        if (l.type != VAL_INT || r.type != VAL_INT) {
            strcpy_s(s->error, "operator requires integers");
            return val_none();
        }
        int a = l.int_val, b = r.int_val;
        switch (n->op) {
            case TOK_PLUS:    return val_int(a + b);
            case TOK_MINUS:   return val_int(a - b);
            case TOK_STAR:    return val_int(a * b);
            case TOK_SLASH:
                if (b == 0) { strcpy_s(s->error, "division by zero"); return val_none(); }
                return val_int(a / b);
            case TOK_PERCENT:
                if (b == 0) { strcpy_s(s->error, "modulo by zero"); return val_none(); }
                return val_int(a % b);
            case TOK_LT:  return val_bool(a <  b);
            case TOK_LE:  return val_bool(a <= b);
            case TOK_GT:  return val_bool(a >  b);
            case TOK_GE:  return val_bool(a >= b);
            default: break;
        }
        strcpy_s(s->error, "unknown operator");
        return val_none();
    }

    case NODE_PRINT: {
        for (int i = 0; i < n->n_children; i++) {
            Value v = eval(s, n->child[i]);
            if (s->error[0]) return val_none();
            char buf[COMOS_MAX_STR];
            val_to_str(v, buf);
            comos_print(buf);
            if (i < n->n_children - 1) comos_print(" ");
        }
        comos_print("\n");
        return val_none();
    }

    // Range Built in function
    case NODE_RANGE:
        return val_int(0);

    case NODE_BLOCK: {
        Value last = val_none();
        for (int i = 0; i < n->n_children; i++) {
            last = eval(s, n->child[i]);
            if (s->error[0] || s->returning || s->breaking || s->continuing)
                return last;
        }
        return last;
    }

    case NODE_IF: {
        Value cond = eval(s, n->child[0]);
        if (s->error[0]) return val_none();
        if (is_truthy(cond)) {
            return eval(s, n->child[1]); // then-body
        } else if (n->n_children == 3) {
            return eval(s, n->child[2]); // elif or else
        }
        return val_none();
    }

    case NODE_WHILE: {
        Value result = val_none();
        while (true) {
            Value cond = eval(s, n->child[0]);
            if (s->error[0] || !is_truthy(cond)) break;
            result = eval(s, n->child[1]);
            if (s->error[0] || s->returning) break;
            if (s->breaking)    { s->breaking    = false; break; }
            if (s->continuing)  { s->continuing  = false; }
        }
        return result;
    }

    case NODE_FOR: {
        Node* iter_node = &s->nodes[n->child[0]];
        int limit = 0;
        if (iter_node->type == NODE_RANGE) {
            if (iter_node->n_children < 1) { strcpy_s(s->error, "range() needs one argument"); return val_none(); }
            Value v = eval(s, iter_node->child[0]);
            if (s->error[0]) return val_none();
            if (v.type != VAL_INT) { strcpy_s(s->error, "range() argument must be integer"); return val_none(); }
            limit = v.int_val;
        } else {
            Value v = eval(s, n->child[0]);
            if (s->error[0]) return val_none();
            if (v.type != VAL_INT) { strcpy_s(s->error, "for loop requires integer or range()"); return val_none(); }
            limit = v.int_val;
        }

        Value result = val_none();
        for (int i = 0; i < limit; i++) {
            if (!set_var(s, n->str_val, val_int(i))) return val_none();
            result = eval(s, n->child[1]);
            if (s->error[0] || s->returning) break;
            if (s->breaking)   { s->breaking   = false; break; }
            if (s->continuing) { s->continuing = false; }
        }
        return result;
    }

    case NODE_DEF: {
        if (s->func_count >= COMOS_MAX_FUNCS) {
            strcpy_s(s->error, "too many function definitions");
            return val_none();
        }
        FuncDef* f = &s->funcs[s->func_count++];
        strcpy_s(f->name, n->str_val);
        f->n_params = 0;
        int body_child = n->n_children - 1;
        for (int i = 0; i < body_child && i < COMOS_MAX_PARAMS; i++) {
            strcpy_s(f->params[f->n_params++], s->nodes[n->child[i]].str_val);
        }
        f->body_node = n->child[body_child];
        return val_none();
    }

    case NODE_CALL: {
        FuncDef* f = find_func(s, n->str_val);
        if (!f) {
            strcpy_s(s->error, "undefined function: ");
            strcat_s(s->error, n->str_val);
            return val_none();
        }
        if (n->n_children != f->n_params) {
            strcpy_s(s->error, "wrong number of arguments");
            return val_none();
        }
        if (s->call_depth >= COMOS_MAX_CALL_DEPTH) {
            strcpy_s(s->error, "max call depth exceeded");
            return val_none();
        }

        Value args[COMOS_MAX_PARAMS];
        for (int i = 0; i < n->n_children; i++) {
            args[i] = eval(s, n->child[i]);
            if (s->error[0]) return val_none();
        }

        s->call_depth++;
        for (int i = 0; i < f->n_params; i++) {
            if (!set_var(s, f->params[i], args[i])) {
                s->call_depth--;
                return val_none();
            }
        }

        Value result = eval(s, f->body_node);
        if (s->returning) {
            result = s->return_val;
            s->returning = false;
        }

        pop_scope(s);
        s->call_depth--;
        return result;
    }

    case NODE_RETURN: {
        Value v = val_none();
        if (n->n_children > 0) {
            v = eval(s, n->child[0]);
            if (s->error[0]) return val_none();
        }
        s->returning  = true;
        s->return_val = v;
        return v;
    }

    case NODE_BREAK:    s->breaking   = true; return val_none();
    case NODE_CONTINUE: s->continuing = true; return val_none();

    default:
        strcpy_s(s->error, "unknown node type");
        return val_none();
    }
}

void comos_init(ComosState* s) {
    s->node_count  = 0;
    s->tok_count   = 0;
    s->tok_pos     = 0;
    s->var_count   = 0;
    s->func_count  = 0;
    s->call_depth  = 0;
    s->returning   = false;
    s->breaking    = false;
    s->continuing  = false;
    s->error[0]    = 0;

    Value none = val_none();
    s->return_val = none;
}

extern bool comos_lex(ComosState* s, const char* src);
extern int  comos_parse(ComosState* s);

bool comos_run(ComosState* s, const char* src) {
    s->error[0]   = 0;
    s->returning  = false;
    s->breaking   = false;
    s->continuing = false;

    // Lex
    if (!comos_lex(s, src)) {
        comos_print("comos lex error: ");
        comos_print(s->error);
        comos_print("\n");
        return false;
    }

    // Parse
    int root = comos_parse(s);
    if (root < 0 || s->error[0]) {
        comos_print("comos parse error: ");
        comos_print(s->error);
        comos_print("\n");
        return false;
    }

    // Interpret
    eval(s, root);
    if (s->error[0]) {
        comos_print("comos runtime error: ");
        comos_print(s->error);
        comos_print("\n");
        return false;
    }

    return true;
}
