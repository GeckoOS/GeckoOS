#ifndef GK_H
#define GK_H

//ember2819
// GeckoOS scripting language (.gk)

// Python-like syntax (Literally python btw)

#ifdef GK_HOSTED
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//ember2819
static inline void gk_print(const char* s) { fputs(s, stdout); }
static inline int gk_hosted_getline(char* buf, int max) {
    int i = 0;
    int c;
    while (i < max - 1) {
        c = fgetc(stdin);
        if (c == EOF || c == '\n') break;
        buf[i++] = (char)c;
    }
    buf[i] = 0;
    return i;
}

#else
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <colors.h>
void printc(char* data, uint8_t color);

static inline void gk_print(const char* s) { printc((char*)s, TERM_COLOR); }
// Forward declaration for kernel input function used by gk input()
void input(unsigned char* buff, size_t buffer_size, uint8_t color);

#endif

#define GK_MAX_TOKENS     2048
#define GK_MAX_NODES      2048
#define GK_MAX_VARS        128
#define GK_MAX_FUNCS        64
#define GK_MAX_PARAMS        8
#define GK_MAX_CALL_DEPTH   32
#define GK_MAX_STR          256
#define GK_MAX_SRC         8192
#define GK_MAX_CHILDREN     64

typedef enum {
    TOK_INT, TOK_STR, TOK_IDENT,
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT,
    TOK_EQ, TOK_NEQ, TOK_LT, TOK_LE, TOK_GT, TOK_GE,
    TOK_AND, TOK_OR, TOK_NOT,
    TOK_ASSIGN,
    // punctuation
    TOK_LPAREN, TOK_RPAREN, TOK_COMMA, TOK_COLON,
    // keywords
    TOK_IF, TOK_ELIF, TOK_ELSE,
    TOK_WHILE, TOK_FOR, TOK_IN,
    TOK_DEF, TOK_RETURN,
    TOK_BREAK, TOK_CONTINUE,
    TOK_TRUE, TOK_FALSE,
    TOK_NEWLINE, TOK_INDENT, TOK_DEDENT,
    TOK_EOF,
} TokenType;

typedef struct {
    TokenType type;
    int       int_val;
    char      str_val[GK_MAX_STR];
    int       line;
} Token;

typedef enum {
    NODE_INT,        // integer literal
    NODE_STR,        // string literal
    NODE_BOOL,       // true / false
    NODE_VAR,        // variable reference
    NODE_ASSIGN,
    NODE_BINOP,
    NODE_UNOP,
    NODE_CALL,
    NODE_IF,         // if / elif / else
    NODE_WHILE,
    NODE_FOR,
    NODE_DEF,
    NODE_RETURN,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_BLOCK,
    NODE_PRINT,
    NODE_RANGE,
    NODE_INPUT,      // input() builtin 
    NODE_INT_CAST,   // int() builtin 
} NodeType;

typedef struct Node Node;

struct Node {
    NodeType type;
    int      line;

    int int_val;

    char str_val[GK_MAX_STR];

    TokenType op;

    int child[GK_MAX_CHILDREN];
    int n_children;
};

typedef enum { VAL_INT, VAL_STR, VAL_BOOL, VAL_NONE } ValType;

typedef struct {
    ValType type;
    int     int_val;
    bool    bool_val;
    char    str_val[GK_MAX_STR];
} Value;

typedef struct {
    char  name[64];
    Value val;
    int   scope;
} Var;

typedef struct {
    char name[64];
    char params[GK_MAX_PARAMS][64];
    int  n_params;
    int  body_node;
} FuncDef;

typedef struct {
    Node nodes[GK_MAX_NODES];
    int  node_count;

    Token tokens[GK_MAX_TOKENS];
    int   tok_count;
    int   tok_pos;

    Var  vars[GK_MAX_VARS];
    int  var_count;

    FuncDef funcs[GK_MAX_FUNCS];
    int     func_count;

    int call_depth;

    bool returning;
    bool breaking;
    bool continuing;
    Value return_val;

    char error[GK_MAX_STR];
} GkState;

void gk_init(GkState* s);

bool gk_run(GkState* s, const char* src);
extern void int_to_str(int v, char* buf);

#endif
