#include <assert.h>
#include <ctype.h>
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

enum TokenKind {
    TOKEN_NONE = 0,
    // first 128 values reserved to match up with ascii
    TOKEN_INT = 128,
    TOKEN_SHIFT_L,
    TOKEN_SHIFT_R,
};

typedef struct Token {
    uint32_t kind;
    uint64_t val;
} Token;

global_variable char *g_stream;

#define ERROR() assert(false && "error")

void print_token(struct Token token) {
    switch(token.kind) {
        case TOKEN_INT:
            printf("TOKEN_INT: %lu\n", token.val);
            break;
        case TOKEN_SHIFT_L:
            printf("TOKEN: '<<'\n");
            break;
        case TOKEN_SHIFT_R:
            printf("TOKEN: '>>'\n");
            break;
        case TOKEN_NONE: // fallthrough
        default:
            printf("TOKEN: '%c'\n", token.kind);
            break;
    }
}

char *lex_next_token(Token *out_t) {
    assert(out_t);

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
                out_t->kind = TOKEN_SHIFT_L;
                cursor++;
            } else {
                ERROR();
            }
            break;
        case '>':
            cursor++;
            if (*cursor == '>') {
                out_t->kind = TOKEN_SHIFT_R;
                cursor++;
            } else {
                ERROR();
            }
            break;
        default:
            out_t->kind = *cursor++;
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
    char *source = "+()_HELLO    1,23<<>>4+FOO!994";
    g_stream = source;
    Token t = token_consume();
    while (t.kind) {
        //print_token(t);
        t = token_consume();
    }
}

enum OpKind {
    OP_NONE,

    OP_ADD,
    OP_SUB,
    OP_BIN_OR,
    OP_BIN_XOR,

    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_SHIFT_L,
    OP_SHIFT_R,
    OP_BIN_AND,

    OP_NEG,
    OP_BIN_NOT,
};

enum ExprKind {
    EXPR_NONE,
    EXPR_OPERAND,
    EXPR_UNARY,
    EXPR_BINARY,
};

bool unary_op(enum OpKind *op) {
    assert(op);
    enum OpKind kind = OP_NONE;

    Token next = token_peek();
    switch (next.kind) {
        case '-':
            kind = OP_NEG;
            break;
        case '~':
            kind = OP_BIN_NOT;
            break;
    }

    if (kind != OP_NONE) {
        *op = kind;
        token_consume();
        return true;
    }

    return false;
}

bool mul_op(enum OpKind *op) {
    assert(op);
    enum OpKind kind = OP_NONE;

    Token next = token_peek();
    switch (next.kind) {
        case '*':
            kind = OP_MUL;
            break;
        case '/':
            kind = OP_DIV;
            break;
        case '%':
            kind = OP_MOD;
            break;
        case TOKEN_SHIFT_L:
            kind = OP_SHIFT_L;
            break;
        case TOKEN_SHIFT_R:
            kind = OP_SHIFT_R;
            break;
    }

    if (kind != OP_NONE) {
        *op = kind;
        token_consume();
        return true;
    }

    return false;
}

bool add_op(enum OpKind *op) {
    assert(op);
    enum OpKind kind = OP_NONE;

    Token next = token_peek();
    switch (next.kind) {
        case '+':
            kind = OP_ADD;
            break;
        case '-':
            kind = OP_SUB;
            break;
        case '|':
            kind = OP_BIN_OR;
            break;
        case '^':
            kind = OP_BIN_XOR;
            break;
    }

    if (kind != OP_NONE) {
        *op = kind;
        token_consume();
        return true;
    }

    return false;
}

typedef struct Expr Expr;

struct Expr {
    enum ExprKind kind;
    enum OpKind op;
    Expr *left;
    Expr *right;
    int32_t operand;
};

Expr *expr_alloc(void) {
    Expr *e = malloc(sizeof(Expr));
    *e = (Expr) {0};
    return e;
}

Expr *expr_unary(enum OpKind o, Expr *u) {
    Expr *e = expr_alloc();
    *e = (Expr) {
        .kind = EXPR_UNARY,
        .op = o,
        .left = u,
    };
    return e;
}

Expr *expr_binary(enum OpKind o, Expr *l, Expr *r) {
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

    char *op = NULL;
    switch (e->op) {
        case OP_NONE:               break;
        case OP_ADD:     op = "+";  break;
        case OP_SUB:     op = "-";  break;
        case OP_BIN_OR:  op = "|";  break;
        case OP_BIN_XOR: op = "^";  break;
        case OP_MUL:     op = "*";  break;
        case OP_DIV:     op = "/";  break;
        case OP_MOD:     op = "%";  break;
        case OP_SHIFT_L: op = "<<"; break;
        case OP_SHIFT_R: op = ">>"; break;
        case OP_BIN_AND: op = "&";  break;
        case OP_NEG:     op = "-";  break;
        case OP_BIN_NOT: op = "~";  break;
        default:
            ERROR();
            break;
    }

    if (e->kind != EXPR_OPERAND) {
        printf("(");
    }

    switch (e->kind) {
        case EXPR_OPERAND:
            printf("%i", e->operand);
            break;
        case EXPR_UNARY:
            if (op) {
                printf("%s ", op);
            }
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
    enum OpKind op = 0;
    if (unary_op(&op)) {
        return expr_unary(op, parse_expr_unary());
    } else {
        return parse_expr_operand();
    }
}

Expr *parse_expr_mul(void) {
    Expr *e = parse_expr_unary();
    enum OpKind op = 0;
    while (mul_op(&op)) {
        e = expr_binary(op, e, parse_expr_unary());
    }
    return e;
}

Expr *parse_expr_add(void) {
    Expr *e = parse_expr_mul();
    enum OpKind op = 0;
    while (add_op(&op)) {
        e = expr_binary(op, e, parse_expr_mul());
    }

    return e;
}

Expr *parse_expr(void) {
    return parse_expr_add();
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
void parse_test(void) {
    char *source = "12*34 + 45/56 + -25";
    printf("%s\n", source);
    g_stream = source;
    Expr *e = parse_expr();
    print_expr(e);
}

int main(void) {
    buf_test();
    lex_test();
    parse_test();
}
