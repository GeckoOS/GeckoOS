#ifndef COMOS_H
#define COMOS_H

// CommunityOS scripting language (.comos)
// Python-like syntax (Literally python btw)

#ifdef COMOS_HOSTED
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static inline void comos_print(const char* s) { fputs(s, stdout); }
#else
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../colors.h"
void printc(char* data, uint8_t color);
static inline void comos_print(const char* s) { printc((char*)s, TERM_COLOR); }
#endif

#define COMOS_MAX_TOKENS     2048
#define COMOS_MAX_NODES      2048
#define COMOS_MAX_VARS        128
#define COMOS_MAX_FUNCS        64
#define COMOS_MAX_PARAMS        8
#define COMOS_MAX_CALL_DEPTH   32
#define COMOS_MAX_STR          256
#define COMOS_MAX_SRC         8192
#define COMOS_MAX_CHILDREN     64

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
    char      str_val[COMOS_MAX_STR];
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
} NodeType;

typedef struct Node Node;

struct Node {
    NodeType type;
    int      line;

    int int_val;

    char str_val[COMOS_MAX_STR];

    TokenType op;

    int child[COMOS_MAX_CHILDREN];
    int n_children;
};

typedef enum { VAL_INT, VAL_STR, VAL_BOOL, VAL_NONE } ValType;

typedef struct {
    ValType type;
    int     int_val;
    bool    bool_val;
    char    str_val[COMOS_MAX_STR];
} Value;

typedef struct {
    char  name[64];
    Value val;
    int   scope;
} Var;

typedef struct {
    char name[64];
    char params[COMOS_MAX_PARAMS][64];
    int  n_params;
    int  body_node;
} FuncDef;

typedef struct {
    Node nodes[COMOS_MAX_NODES];
    int  node_count;

    Token tokens[COMOS_MAX_TOKENS];
    int   tok_count;
    int   tok_pos;

    Var  vars[COMOS_MAX_VARS];
    int  var_count;

    FuncDef funcs[COMOS_MAX_FUNCS];
    int     func_count;

    int call_depth;

    bool returning;
    bool breaking;
    bool continuing;
    Value return_val;

    char error[COMOS_MAX_STR];
} ComosState;

void comos_init(ComosState* s);

bool comos_run(ComosState* s, const char* src);
extern void int_to_str(int v, char* buf);

#endif
