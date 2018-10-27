#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

int main(void)
{
    buf_test();
}
