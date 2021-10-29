#ifndef _FRAME_H
#define _FRAME_H

/**
 * File Tranfer Frame Format - Dao Minh Hai (barrydevp)
 *
 * contain message format, utils function
 *
 *                    0      1      2      3
 *                +-----------------------------+ -+
 *              0 |            SIZE             |  |
 *                ---------------+-------+-------  |  Base Header (8) bytes
 *              4 |   OPERATION  |  OFF  | FLAG |  |
 *                ---------------+-------+------- -|
 *         FROM 8 |          EXTENSIONS         |  |  Extension Header (OFF * 4 - 8) bytes
 *                ------------------------------- -|
 *     0 + OFF * 4|                             |  |
 *                |                             |  |
 *                |                             |  |
 *                |                             |  |
 *                |            DATA             |  |  Payload/Body (SIZE - OFF * 4) bytes
 *                |                             |  |
 *                |                             |  |
 *                |                             |  |
 *                |                             |  |
 *                |                             |  |
 *                +-----------------------------+ -+
 *
 *
 */

#include "stdint.h"
#include "string.h"

#define BASE_HEADER_SIZE 8
#define MAX_HEADER_SIZE 1024
#define EXTENSION_SIZE 4  // 1 extension entry = 4 bytes
#define OFF_RATE 4        // 1 offset = 4 bytes
#define DEFAULT_BODY_BUF_SIZE 1024
#define MAX_FRAME_SIZE 0xffffffff

// state statuses
typedef enum {
    STATUS_OK = 0,

    STATUS_ERR = 1,

    STATUS_CLOSE = 9,
} status;

// OP CODE
typedef enum {
    OP_INVALID = 0,

    OP_HELLO = 100,

    OP_GET_FILE = 200,

    OP_QUIT = 900,
} opcode;

// BASIC OP FLAG
enum {
    FL_REQUEST = 0,

    FL_ERROR = 1,

    FL_SUCCESS = 2,
};

// GET_FILE OP FLAG
enum {
    FL_GF_IN_TRANSIT = 3,

    FL_GF_END = 4,
};

typedef char buf;
typedef buf* ext;  // extesion must be 4 bytes => char[4]

typedef struct {
    /* it's buffer! */
    buf* buffers;

    /* actually allocated buffers size */
    uint32_t buffer_size;

    /* number of already readed bytes */
    uint32_t len;

    /* number of bytes contain in buffers for now */
    uint32_t cur_len;

} frame_body;

typedef struct {
    /* Base Header */
    uint32_t size;
    uint16_t op;
    uint8_t off;
    uint8_t flag;

    /* Extension Header */
    ext* exts;

    /* body */
    frame_body body;

} frame;

static inline int is_bigendian(void) {
    union {
        uint32_t i;
        char c[4];
    } bint = {0x01020304};
    return bint.c[0] == 1;
}

static inline void buf_e8(uint8_t val, buf* data) { memcpy(data, &val, sizeof(val)); }

static inline uint8_t buf_d8(void* data) {
    uint8_t val;
    memcpy(&val, data, sizeof(val));
    return val;
}

static inline void buf_e16(uint16_t val, buf* data) {
    if (!is_bigendian()) {
        val = ((val & 0xFF00u) >> 8u) | ((val & 0x00FFu) << 8u);
    }
    memcpy(data, &val, sizeof(val));
}

static inline uint16_t buf_d16(buf* data) {
    uint16_t val;
    memcpy(&val, data, sizeof(val));
    if (!is_bigendian()) {
        val = ((val & 0xFF00u) >> 8u) | ((val & 0x00FFu) << 8u);
    }
    return val;
}

static inline void buf_e32(uint32_t val, buf* data) {
    if (!is_bigendian()) {
        val = ((val & 0xFF000000u) >> 24u) | ((val & 0x00FF0000u) >> 8u) | ((val & 0x0000FF00u) << 8u) |
              ((val & 0x000000FFu) << 24u);
    }
    memcpy(data, &val, sizeof(val));
}

static inline uint32_t buf_d32(buf* data) {
    uint32_t val;
    memcpy(&val, data, sizeof(val));
    if (!is_bigendian()) {
        val = ((val & 0xFF000000u) >> 24u) | ((val & 0x00FF0000u) >> 8u) | ((val & 0x0000FF00u) << 8u) |
              ((val & 0x000000FFu) << 24u);
    }
    return val;
}

// core function

uint32_t get_frame_header_size(frame* _frame);
uint32_t get_frame_body_size(frame* _frame);
uint32_t get_frame_exts_hdr_size(frame* _frame);

status calc_frame_size(frame* _frame, uint32_t body_size);
status put_body_frame(frame* _frame, buf* buffers, uint32_t buf_size);
status empty_frame(frame* _frame);
status new_empty_frame(frame* _frame, uint16_t _op, uint8_t _flag, uint32_t body_size);
status new_base_frame(frame* _frame, uint16_t _op, uint8_t _flag, buf* data, uint32_t data_size);
void print_frame(frame* _frame);

status rstream_start(int sockfd, frame* _frame);
status rstream_read(int sockfd, frame* _frame, uint32_t data_size, uint32_t* readed_size, uint8_t* next);
status rstream_end(int sockfd, frame* _frame);
status read_frame(int sockfd, frame* _frame);

status wstream_start(int sockfd, frame* _frame);
status wstream_write(int sockfd, frame* _frame, buf* data, uint32_t data_size, uint8_t* next);
status wstream_end(int sockfd, frame* _frame);
status write_frame(int sockfd, frame* _frame);

#endif
