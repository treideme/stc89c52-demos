#ifndef _circular_h_
#define _circular_h_

#include <stdint.h>

#define CIRCULAR_BUF_SZ 40

typedef struct {
    char buf[CIRCULAR_BUF_SZ];
    uint8_t read_idx;
    uint8_t write_idx;
} circular_buf_t;

#define circular_buffer_full(pBuf) \
    (((pBuf)->write_idx == (pBuf)->read_idx-1) || \
     (((pBuf)->write_idx == CIRCULAR_BUF_SZ)&&((pBuf)->read_idx==0)))

#define circular_buffer_empty(pBuf) \
    ((pBuf)->write_idx == (pBuf)->read_idx)

#define circular_peek(pBuf) \
    ((pBuf)->buf[(pBuf)->read_idx])

int8_t circular_put(circular_buf_t*buf, const char d);
//char circular_peek(const circular_buf_t*buf);
int8_t circular_pop(circular_buf_t*buf);


#endif /* _circular_h_ */
