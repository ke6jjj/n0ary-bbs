#include "bbslib.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

struct AsyncLineBuffer *
async_line_new(size_t capacity, int filter_cr)
{
    struct AsyncLineBuffer *b;

    b = malloc(sizeof(struct AsyncLineBuffer));
    if (b == NULL)
        goto AllocFailed;

    if ((b->buf = malloc(capacity)) == NULL)
        goto BufAllocFailed;

    b->alloc = capacity;
    b->cur = 0;
    b->read = 0;
    b->filter_cr = filter_cr;

    return b;

BufAllocFailed:
    free(b);
AllocFailed:
    return NULL;
}

void
async_line_free(struct AsyncLineBuffer *b)
{
    free(b->buf);
    free(b);
}

int
async_line_get(struct AsyncLineBuffer *b, int fd, char **ret)
{
    char *nl;

    for (;;) {
        if (b->cur > 0 && b->cur == b->read) {
            /* All consumed, reset the buffer */
            b->cur = 0;
            b->read = 0;
        }

        if ((nl = memchr(&b->buf[b->cur], '\n', b->read)) != NULL) {
            size_t found = nl - &b->buf[0];
            if (found > b->cur && b->filter_cr) {
                if (*(nl - 1) == '\r') {
                    nl = nl - 1;
                }
            }
               
            *nl = '\0';
            *ret = &b->buf[b->cur];
            b->cur = found + 1;

            return ASYNC_OK;
        }

        if (b->cur > 0) {
            /*
             * Justify the buffer so that it can accomodate the largest
             * read possible.
             */
            memmove(&b->buf[0], &b->buf[b->cur], b->read - b->cur);
            b->read -= b->cur;
            b->cur = 0;
        }

        
        size_t remain = b->alloc - b->read;
        if (remain == 0) {
            return ASYNC_READ_FULL;
        }

        ssize_t nread = read(fd, &b->buf[b->read], remain);
        if (nread == 0) {
            /* End of file */
            return ASYNC_READ_ERROR;
        }
        if (nread < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return ASYNC_MORE;
            }
            return ASYNC_READ_ERROR;
        }

        b->read += nread;
    }
}
