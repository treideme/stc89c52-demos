#ifndef _burst_h
#include <stddef.h>
#include <stdint.h>

/**
 * Burst buffer structure.
 *  Assign a pointer array and then iterate.
 */
typedef struct {
    uint8_t len;
    uint8_t i;
    uint8_t*data;
} burst_buf_t;

#define burst_empty(buf) (buf.i==buf.len)
#define burst_init(buf, d, sz) \
    buf.data = (d); \
    buf.i = 0;      \
    buf.len = (sz)

#define burst_peek(buf) \
    buf.data[buf.i]

#define burst_pop(buf) \
    buf.i++


#endif // _burst_h
