#include "circular.h"

int8_t circular_put(circular_buf_t*buf, const char d) {
    // Alarm buffer congestion
    if(circular_buffer_full(buf))
        return -1;

    // Append
    buf->buf[buf->write_idx++] = d;
    if(buf->write_idx == CIRCULAR_BUF_SZ)
        buf->write_idx = 0;

    return 0;
}

//char circular_peek(const circular_buf_t*buf) {
//    return buf->buf[buf->read_idx];
//}

int8_t circular_pop(circular_buf_t*buf) {
    if(circular_buffer_empty(buf))
        return -1;

    buf->read_idx++;

    if(buf->read_idx == CIRCULAR_BUF_SZ)
        buf->read_idx = 0;

    return 0;
}
