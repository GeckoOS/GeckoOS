//ember2819
// Renamed from comos_lexer.c to gk_lexer.c
// Updated all ComoState/COMOS references to GkState/GK
//ember2819 end
#include <gk/gk.h>

static bool is_digit(char c)  { return c >= '0' && c <= '9'; }
static bool is_alpha(char c)  { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
static bool is_alnum(char c)  { return is_alpha(c) || is_digit(c); }
static bool is_space(char c)  { return c == ' ' || c == '\t'; }

static int gk_strlen(const char* s) {
    int n = 0; while (s[n]) n++; return n;
}
static void gk_strcpy(char* dst, const char* src) {
    int i = 0; while (src[i]) { dst[i] = src[i]; i++; } dst[i] = 0;
}
static bool gk_streq(const char* a, const char* b) {
    int i = 0;
    while (a[i] && b[i]) { if (a[i] != b[i]) return false; i++; }
    return a[i] == b[i];
}

static struct { const char* word; TokenType type; } KEYWORDS[] = {
    {"if",       TOK_IF},
    {"elif",     TOK_ELIF},
    {"else",     TOK_ELSE},
    {"while",    TOK_WHILE},
    {"for",      TOK_FOR},
    {"in",       TOK_IN},
    {"def",      TOK_DEF},
    {"return",   TOK_RETURN},
    {"break",    TOK_BREAK},
    {"continue", TOK_CONTINUE},
    {"and",      TOK_AND},
    {"or",       TOK_OR},
    {"not",      TOK_NOT},
    {"True",     TOK_TRUE},
    {"False",    TOK_FALSE},
    {0, 0}
};

#define MAX_INDENT 64
static int indent_stack[MAX_INDENT];
static int indent_top;

static bool emit(GkState* s, TokenType t, int line) {
    if (s->tok_count >= GK_MAX_TOKENS) {
        gk_strcpy(s->error, "too many tokens");
        return false;
    }
    s->tokens[s->tok_count].type = t;
    s->tokens[s->tok_count].line = line;
    s->tokens[s->tok_count].str_val[0] = 0;
    s->tokens[s->tok_count].int_val = 0;
    s->tok_count++;
    return true;
}

static bool emit_int(GkState* s, int v, int line) {
    if (!emit(s, TOK_INT, line)) return false;
    s->tokens[s->tok_count - 1].int_val = v;
    return true;
}

static bool emit_str(GkState* s, TokenType t, const char* str, int line) {
    if (!emit(s, t, line)) return false;
    gk_strcpy(s->tokens[s->tok_count - 1].str_val, str);
    return true;
}

bool gk_lex(GkState* s, const char* src) {
    s->tok_count = 0;
    s->tok_pos   = 0;

    indent_stack[0] = 0;
    indent_top      = 0;

    int i    = 0;
    int line = 1;
    int src_len = gk_strlen(src);

    while (i < src_len) {
        int indent = 0;
        while (i < src_len && src[i] == ' ')  { indent++; i++; }
        while (i < src_len && src[i] == '\t') { indent += 4; i++; }

        if (i >= src_len || src[i] == '\n' || src[i] == '#') {
            while (i < src_len && src[i] != '\n') i++;
            if (i < src_len) { i++; line++; }
            continue;
        }

        if (indent > indent_stack[indent_top]) {
            indent_stack[++indent_top] = indent;
            if (!emit(s, TOK_INDENT, line)) return false;
        } else {
            while (indent < indent_stack[indent_top]) {
                indent_top--;
                if (!emit(s, TOK_DEDENT, line)) return false;
            }
        }

        while (i < src_len && src[i] != '\n') {
            char c = src[i];

            if (is_space(c)) { i++; continue; }

            if (c == '#') {
                while (i < src_len && src[i] != '\n') i++;
                continue;
            }

            if (is_digit(c)) {
                int val = 0;
                while (i < src_len && is_digit(src[i])) {
                    val = val * 10 + (src[i] - '0');
                    i++;
                }
                if (!emit_int(s, val, line)) return false;
                continue;
            }

            if (c == '"' || c == '\'') {
                char quote = c;
                i++;
                char buf[GK_MAX_STR];
                int  bi = 0;
                while (i < src_len && src[i] != quote) {
                    if (src[i] == '\\' && i + 1 < src_len) {
                        i++;
                        switch (src[i]) {
                            case 'n':  buf[bi++] = '\n'; break;
                            case 't':  buf[bi++] = '\t'; break;
                            case '\\': buf[bi++] = '\\'; break;
                            default:   buf[bi++] = src[i]; break;
                        }
                    } else {
                        buf[bi++] = src[i];
                    }
                    i++;
                    if (bi >= GK_MAX_STR - 1) break;
                }
                buf[bi] = 0;
                if (i < src_len) i++;
                if (!emit_str(s, TOK_STR, buf, line)) return false;
                continue;
            }

            if (is_alpha(c)) {
                char buf[64];
                int  bi = 0;
                while (i < src_len && is_alnum(src[i]) && bi < 63) {
                    buf[bi++] = src[i++];
                }
                buf[bi] = 0;

                // check keyword table
                TokenType kw = TOK_IDENT;
                for (int k = 0; KEYWORDS[k].word; k++) {
                    if (gk_streq(buf, KEYWORDS[k].word)) {
                        kw = KEYWORDS[k].type;
                        break;
                    }
                }
                if (!emit_str(s, kw, buf, line)) return false;
                continue;
            }

            char next = (i + 1 < src_len) ? src[i + 1] : 0;
            if (c == '=' && next == '=') { emit(s, TOK_EQ,  line); i += 2; continue; }
            if (c == '!' && next == '=') { emit(s, TOK_NEQ, line); i += 2; continue; }
            if (c == '<' && next == '=') { emit(s, TOK_LE,  line); i += 2; continue; }
            if (c == '>' && next == '=') { emit(s, TOK_GE,  line); i += 2; continue; }

            // single-character operators and punctuation
            switch (c) {
                case '+': emit(s, TOK_PLUS,   line); break;
                case '-': emit(s, TOK_MINUS,  line); break;
                case '*': emit(s, TOK_STAR,   line); break;
                case '/': emit(s, TOK_SLASH,  line); break;
                case '%': emit(s, TOK_PERCENT,line); break;
                case '<': emit(s, TOK_LT,     line); break;
                case '>': emit(s, TOK_GT,     line); break;
                case '=': emit(s, TOK_ASSIGN, line); break;
                case '(': emit(s, TOK_LPAREN, line); break;
                case ')': emit(s, TOK_RPAREN, line); break;
                case ',': emit(s, TOK_COMMA,  line); break;
                case ':': emit(s, TOK_COLON,  line); break;
                default:
                    s->error[0] = 'U'; s->error[1] = 'n'; s->error[2] = 'k';
                    s->error[3] = 0;
                    return false;
            }
            i++;
        }

        if (!emit(s, TOK_NEWLINE, line)) return false;
        if (i < src_len) { i++; line++; }
    }

    while (indent_top > 0) {
        indent_top--;
        if (!emit(s, TOK_DEDENT, line)) return false;
    }

    if (!emit(s, TOK_EOF, line)) return false;
    return true;
}
