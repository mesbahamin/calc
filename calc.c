#include <assert.h>
#include <ctype.h>
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

void *cmalloc(size_t num_bytes)
{
    void *ptr = malloc(num_bytes);
    if (!ptr) {
        fprintf(stderr, "crealloc failed\n");
        exit(1);
    }
    return ptr;
}

void *crealloc(void *ptr, size_t num_bytes)
{
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

void *buf__grow(void *buf, size_t new_length, size_t element_size)
{
    size_t new_cap = MAX(1 + (2 * buf_cap(buf)), new_length);
    assert(new_cap >= new_length);
    size_t new_size = offsetof(struct BufHdr, buf) + (new_cap * element_size);
    struct BufHdr *new_hdr = NULL;
    if (buf)
    {
        new_hdr = crealloc(buf__hdr(buf), new_size);
    }
    else
    {
        new_hdr = cmalloc(new_size);
        new_hdr->len = 0;
    }
    new_hdr->cap = new_cap;
    return new_hdr->buf;
}

void buf_test(void)
{
    int *buf = NULL;
    assert(buf_len(buf) == 0);
    for (size_t i = 0; i < 1000; ++i)
    {
        buf_push(buf, i);
    }
    assert(buf_len(buf) == 1000);
    for (size_t i = 0; i < buf_len(buf); ++i)
    {
        printf("%d, ", buf[i]);
    }
    printf("\n");
}

enum TokenKind {
    // first 128 values reserved to match up with ascii
    TOKEN_INT = 128,
    TOKEN_NAME,
};

struct Token {
    enum TokenKind kind;
    union {
        uint64_t val;
        struct {
            const char *start;
            const char *end;
        };
    };
};

global_variable struct Token g_token;
global_variable const char *g_stream;

void lex_next_token() {
    switch(*g_stream)
    {
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
            while (isdigit(*g_stream))
            {
                val *= 10;
                val += *g_stream++ - '0';
            }
            g_token.kind = TOKEN_INT;
            g_token.val = val;
            break;
        }
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
        case 'i':
        case 'j':
        case 'k':
        case 'l':
        case 'm':
        case 'n':
        case 'o':
        case 'p':
        case 'q':
        case 'r':
        case 's':
        case 't':
        case 'u':
        case 'v':
        case 'w':
        case 'x':
        case 'y':
        case 'z':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'I':
        case 'J':
        case 'K':
        case 'L':
        case 'M':
        case 'N':
        case 'O':
        case 'P':
        case 'Q':
        case 'R':
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
        case 'Y':
        case 'Z':
        case '_': {
            const char *start = g_stream++;
            while (isalnum(*g_stream) || *g_stream == '_')
            {
                g_stream++;
            }
            g_token.kind = TOKEN_NAME;
            g_token.start = start;
            g_token.end = g_stream;
            break;
        }
        default:
            g_token.kind = *g_stream++;
            break;
    }
}

void print_token(struct Token token)
{
    switch(token.kind)
    {
        case TOKEN_INT:
            printf("TOKEN_INT: %lu\n", token.val);
            break;
        case TOKEN_NAME:
            printf("TOKEN_NAME: %.*s\n", (int)(token.end - token.start), token.start);
            break;
        default:
            printf("TOKEN: '%c'\n", token.kind);
            break;
    }
}

void lex_test()
{
    char *source = "+()_HELLO1,234+FOO!994";
    g_stream = source;
    lex_next_token();
    while (g_token.kind)
    {
        print_token(g_token);
        lex_next_token();
    }
}

int main(void)
{
    buf_test();
    lex_test();
}
