#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define global_variable static

struct BufHdr {
    size_t cap;
    size_t len;
    uint8_t buf[];
};

void *cmalloc(size_t num_bytes) {
    void *ptr = malloc(num_bytes);
    if (!ptr) {
        fprintf(stderr, "crealloc failed\n");
        exit(1);
    }
    return ptr;
}

void *crealloc(void *ptr, size_t num_bytes) {
    ptr = realloc(ptr, num_bytes);
    if (!ptr) {
        fprintf(stderr, "crealloc failed\n");
        exit(1);
    }
    return ptr;
}

#define buf__hdr(b) ((struct BufHdr *)((uint8_t *)(b) - offsetof(struct BufHdr, buf)))
#define buf_cap(b) ((b) ? buf__hdr(b)->cap : 0)
#define buf_len(b) ((b) ? buf__hdr(b)->len : 0)

#define buf__must_grow(b, n) (buf_len(b) + (n) >= buf_cap(b))
#define buf__maybe_grow(b, n) (buf__must_grow((b), (n)) ? (b) = buf__grow((b), buf_len(b) + (n), sizeof(*(b))) : 0)
#define buf_push(b, x) (buf__maybe_grow((b), 1), (b)[buf__hdr(b)->len++] = (x))

#define MAX(x, y) ((x) >= (y) ? (x) : (y))

void *buf__grow(void *buf, size_t new_length, size_t element_size) {
    size_t new_cap = MAX(1 + (2 * buf_cap(buf)), new_length);
    assert(new_cap >= new_length);
    size_t new_size = offsetof(struct BufHdr, buf) + (new_cap * element_size);
    struct BufHdr *new_hdr = NULL;
    if (buf) {
        new_hdr = crealloc(buf__hdr(buf), new_size);
    } else {
        new_hdr = cmalloc(new_size);
        new_hdr->len = 0;
    }
    new_hdr->cap = new_cap;
    return new_hdr->buf;
}

void buf_test(void) {
    int *buf = NULL;
    assert(buf_len(buf) == 0);
    for (size_t i = 0; i < 1000; ++i) {
        buf_push(buf, i);
    }
    assert(buf_len(buf) == 1000);
    for (size_t i = 0; i < buf_len(buf); ++i) {
        //printf("%d, ", buf[i]);
    }
    //printf("\n");
}

#define TOKEN_LIST            \
    X(TOKEN_NONE,       "")   \
    X(TOKEN_OP_ADD,     "+")  \
    X(TOKEN_OP_SUB,     "-")  \
    X(TOKEN_OP_BIN_OR,  "|")  \
    X(TOKEN_OP_BIN_XOR, "^")  \
    X(TOKEN_OP_MUL,     "*")  \
    X(TOKEN_OP_DIV,     "/")  \
    X(TOKEN_OP_MOD,     "%")  \
    X(TOKEN_OP_SHIFT_L, "<<") \
    X(TOKEN_OP_SHIFT_R, ">>") \
    X(TOKEN_OP_BIN_AND, "&")  \
    X(TOKEN_OP_BIN_NOT, "~")  \
    X(TOKEN_INT,        "")   \
                              \
    X(_TOKEN_KIND_COUNT, "")

enum TokenKind {
#define X(kind, glyph) kind,
    TOKEN_LIST
#undef X
};

char *TokenKindNames[] = {
#define X(kind, glyph) [kind] = #kind,
    TOKEN_LIST
#undef X
};

char *TokenKindGlyphs[] = {
#define X(kind, glyph) [kind] = glyph,
    TOKEN_LIST
#undef X
};

#undef TOKEN_LIST

char *token_kind_glyph(enum TokenKind k) {
    assert(k >= TOKEN_NONE && k < TOKEN_INT);
    assert(k <= (sizeof TokenKindNames / sizeof TokenKindGlyphs[0]));
    return TokenKindGlyphs[k];
}

char *token_kind_name(enum TokenKind k) {
    assert(k >= TOKEN_NONE && k < _TOKEN_KIND_COUNT);
    assert(k <= (sizeof TokenKindNames / sizeof TokenKindNames[0]));
    return TokenKindNames[k];
}

typedef struct Token {
    enum TokenKind kind;
    uint64_t val;
} Token;

global_variable char *g_stream;

#define ERROR() assert(false && "error")

void print_token(struct Token token) {
    bool has_val = token.kind == TOKEN_INT;
    if (has_val) {
        printf("T: %s: %" PRIu64 "\n", token_kind_name(token.kind), token.val);
    } else {
        printf("T: %s\n", token_kind_name(token.kind));
    }
}

char *lex_next_token(Token *out_t) {
    assert(out_t);

    // TODO: bounds check cursor advances
    char *cursor = g_stream;

    while (*cursor == ' ') {
        cursor++;
    }

    switch(*cursor) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            uint64_t val = 0;
            while (isdigit(*cursor)) {
                val *= 10;
                val += *cursor++ - '0';
            }
            out_t->kind = TOKEN_INT;
            out_t->val = val;
            break;
        }
        case '<':
            cursor++;
            if (*cursor == '<') {
                out_t->kind = TOKEN_OP_SHIFT_L;
                cursor++;
            } else {
                ERROR();
            }
            break;
        case '>':
            cursor++;
            if (*cursor == '>') {
                out_t->kind = TOKEN_OP_SHIFT_R;
                cursor++;
            } else {
                ERROR();
            }
            break;
        case '+': out_t->kind = TOKEN_OP_ADD;     cursor++; break;
        case '-': out_t->kind = TOKEN_OP_SUB;     cursor++; break;
        case '|': out_t->kind = TOKEN_OP_BIN_OR;  cursor++; break;
        case '^': out_t->kind = TOKEN_OP_BIN_XOR; cursor++; break;
        case '*': out_t->kind = TOKEN_OP_MUL;     cursor++; break;
        case '/': out_t->kind = TOKEN_OP_DIV;     cursor++; break;
        case '%': out_t->kind = TOKEN_OP_MOD;     cursor++; break;
        case '&': out_t->kind = TOKEN_OP_BIN_AND; cursor++; break;
        case '~': out_t->kind = TOKEN_OP_BIN_NOT; cursor++; break;
        case '\0':
            out_t->kind = TOKEN_NONE;
            break;
        default:
            printf("ERROR: '%c' %i\n", *cursor, (int)*cursor);
            ERROR();
            break;
    }

    return cursor;
}

bool token_is_kind(Token t, enum TokenKind kind) {
    return t.kind == kind;
}

Token token_peek(void) {
    Token t = {0};
    lex_next_token(&t);
    return t;
}

Token token_consume(void) {
    Token t = {0};
    g_stream = lex_next_token(&t);
    return t;
}

Token token_consume_kind(enum TokenKind kind) {
    Token t = token_peek();
    if (token_is_kind(t, kind)) {
        token_consume();
    } else {
        t = (Token){0};
    }
    return t;
}

Token token_require_kind(enum TokenKind kind) {
    Token t = token_consume();
    if (!token_is_kind(t, kind)) {
        ERROR();
    }
    return t;
}

void lex_test(void) {
    char *source = "+123<<>>4+   ~994";
    printf("lex_test: '%s'\n", source);
    g_stream = source;
    Token t = token_consume();
    while (t.kind) {
        print_token(t);
        t = token_consume();
    }
}

enum ExprKind {
    EXPR_NONE,
    EXPR_OPERAND,
    EXPR_UNARY,
    EXPR_BINARY,
};

bool is_unary_op(enum TokenKind o) {
    return o == TOKEN_OP_SUB || o == TOKEN_OP_BIN_NOT;
}

bool is_mul_op(enum TokenKind o) {
    return o == TOKEN_OP_MUL || o == TOKEN_OP_DIV || o == TOKEN_OP_MOD || o == TOKEN_OP_SHIFT_L || o == TOKEN_OP_SHIFT_R || o == TOKEN_OP_BIN_AND;
}

bool is_add_op(enum TokenKind o) {
    return o == TOKEN_OP_ADD || o == TOKEN_OP_SUB || o == TOKEN_OP_BIN_OR || o == TOKEN_OP_BIN_XOR;
}

typedef struct Expr Expr;

struct Expr {
    enum ExprKind kind;
    enum TokenKind op;
    Expr *left;
    Expr *right;
    int32_t operand;
};

Expr *expr_alloc(void) {
    Expr *e = malloc(sizeof(Expr));
    *e = (Expr) {0};
    return e;
}

Expr *expr_unary(enum TokenKind o, Expr *u) {
    assert(is_unary_op(o));
    Expr *e = expr_alloc();
    *e = (Expr) {
        .kind = EXPR_UNARY,
        .op = o,
        .left = u,
    };
    return e;
}

Expr *expr_binary(enum TokenKind o, Expr *l, Expr *r) {
    assert(is_mul_op(o) || is_add_op(o));
    Expr *e = expr_alloc();
    *e = (Expr) {
        .kind = EXPR_BINARY,
        .op = o,
        .left = l,
        .right = r,
    };
    return e;
}

void print_expr(Expr *e) {
    assert(e);

    char *op = token_kind_glyph(e->op);

    if (e->kind != EXPR_OPERAND) {
        printf("(");
    }

    switch (e->kind) {
        case EXPR_OPERAND:
            printf("%i", e->operand);
            break;
        case EXPR_UNARY:
            printf("%s ", op);
            print_expr(e->left);
            break;
        case EXPR_BINARY:
            printf("%s ", op);
            print_expr(e->left);
            printf(" ");
            print_expr(e->right);
            break;
        case EXPR_NONE: // fallthrough
        default:
            ERROR();
            break;
    }

    if (e->kind != EXPR_OPERAND) {
        printf(")");
    }
}

int32_t parse_number(void) {
    Token t = token_require_kind(TOKEN_INT);
    return t.val;
}

Expr *parse_expr_operand(void) {
    Expr *e = expr_alloc();
    *e = (Expr) {
        .operand = parse_number(),
        .kind = EXPR_OPERAND,
    };
    return e;
}

Expr *parse_expr_unary(void) {
    if (is_unary_op(token_peek().kind)) {
        enum TokenKind op = token_consume().kind;
        return expr_unary(op, parse_expr_unary());
    } else {
        return parse_expr_operand();
    }
}

Expr *parse_expr_mul(void) {
    Expr *e = parse_expr_unary();
    while (is_mul_op(token_peek().kind)) {
        enum TokenKind op = token_consume().kind;
        e = expr_binary(op, e, parse_expr_unary());
    }
    return e;
}

Expr *parse_expr_add(void) {
    Expr *e = parse_expr_mul();
    while (is_add_op(token_peek().kind)) {
        enum TokenKind op = token_consume().kind;
        e = expr_binary(op, e, parse_expr_mul());
    }

    return e;
}

Expr *parse_expr(void) {
    return parse_expr_add();
}

int32_t eval_expr(Expr *e) {
    assert(e);

    switch (e->kind) {
        case EXPR_OPERAND:
            return e->operand;
            break;
        case EXPR_UNARY: {
            int32_t val = eval_expr(e->left);
            switch (e->op) {
                case TOKEN_OP_SUB:
                    return -val;
                    break;
                case TOKEN_OP_BIN_NOT:
                    return ~val;
                    break;
                default:
                    ERROR();
                    break;
            }
        } break;
        case EXPR_BINARY: {
            int32_t val1 = eval_expr(e->left);
            int32_t val2 = eval_expr(e->right);

            switch (e->op) {
                case TOKEN_OP_ADD:
                    return val1 + val2;
                    break;
                case TOKEN_OP_SUB:
                    return val1 - val2;
                    break;
                case TOKEN_OP_BIN_OR:
                    return val1 | val2;
                    break;
                case TOKEN_OP_BIN_XOR:
                    return val1 ^ val2;
                    break;
                case TOKEN_OP_MUL:
                    return val1 * val2;
                    break;
                case TOKEN_OP_DIV:
                    if (val2 == 0) {
                        ERROR();
                    }
                    return val1 / val2;
                    break;
                case TOKEN_OP_MOD:
                    return val1 % val2;
                    break;
                case TOKEN_OP_SHIFT_L:
                    return val1 << val2;
                    break;
                case TOKEN_OP_SHIFT_R:
                    return val1 >> val2;
                    break;
                case TOKEN_OP_BIN_AND:
                    return val1 & val2;
                    break;
                default:
                    ERROR();
                    break;
            }
        } break;
        case EXPR_NONE: // fallthrough
        default:
            ERROR();
            break;
    }
}

// precedence climbing
// op_unary = "-" | "~".
// op_mul = "*" | "/" | "%" | "<<" | ">>" | "&".
// op_add = "+" | "-" | "|" | "^".
// exp_operand = number.
// expr_unary = ([op_unary] exp_unary) | exp_base.
// expr_mul = expr_unary {op_mul expr_unary}.
// expr_add = expr_mul {op_add expr_mul}.
// expr = expr_add.
// number = "0" .. "9" {"0" .. "9"}.
Expr *parse_test(void) {
    char *source = "12*34 + 45/56 + ~-25";
    printf("parse_test: '%s'\n", source);
    g_stream = source;
    Expr *e = parse_expr();
    print_expr(e);
    printf("\n");
    return e;
}

void eval_test(Expr *e) {
    printf("Result: %i\n", eval_expr(e));
}

int main(void) {
    buf_test();
    lex_test();
    Expr *e = parse_test();
    eval_test(e);
    return 0;
}
