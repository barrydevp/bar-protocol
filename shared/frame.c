#include "frame.h"

#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "clog.h"

status validate_args(frame* _frame, buf* buffers) {
    if (_frame == NULL) {
        errorf("_frame is null");
        return STATUS_ERR;
    }

    if (buffers == NULL) {
        errorf("buffers is null");
        return STATUS_ERR;
    }

    return STATUS_OK;
}

status validate_frame(frame* _frame) {
    status s = STATUS_OK;

    if (_frame == NULL) {
        errorf("_frame is null");
        return STATUS_ERR;
    }

    return s;
}

uint32_t get_frame_header_size(frame* _frame) { return _frame->off * OFF_RATE; }
uint32_t get_frame_body_size(frame* _frame) { return _frame->size - get_frame_header_size(_frame); }
uint32_t get_frame_exts_hdr_size(frame* _frame) { return get_frame_header_size(_frame) - BASE_HEADER_SIZE; }

status _free_exts_hdr(frame* _frame) {
    status s = STATUS_OK;

    if (_frame == NULL || _frame->exts == NULL) {
        return s;
    }

    // free old extension
    for (uint8_t i = 0; i < _frame->off - 2; ++i) {
        free(*(_frame->exts + i));
    }

    // free exts pointer
    free(_frame->exts);

    return s;
}

status _alloc_exts_hdr(frame* _frame) {
    // warning yout must call _free_exts_hdr first after construct new frame

    // alloc new memory for exts pointer
    _frame->exts = (ext*)malloc(sizeof(ext*) * (_frame->off - 2));
    if (_frame->exts == NULL) {
        errorf("cannot malloc new header extentions pointer");
        return STATUS_ERR;
    }

    // alloc new memory for extension

    for (uint8_t i = 0; i < _frame->off - 2; ++i) {
        *(_frame->exts + i) = (ext)malloc(sizeof(buf) * EXTENSION_SIZE);

        if (_frame->exts == NULL) {
            /* @todo clear all alloc mem for previous extension */
            errorf("cannot malloc new header extension");
            return STATUS_ERR;
        }
    }

    return STATUS_OK;
}

status _free_body(frame* _frame) {
    status s = STATUS_OK;

    if (_frame == NULL || (_frame->body).buffers == NULL) {
        return s;
    }

    frame_body* _body = &(_frame->body);

    free(_body->buffers);

    _body->len = 0;

    return s;
}

status _realloc_body(frame* _frame, uint32_t alloc_size) {
    status s = STATUS_OK;

    // if alloc_size request < Default -> we will allocate default size to prevent many call to allocate
    alloc_size = alloc_size > DEFAULT_BODY_BUF_SIZE ? alloc_size : DEFAULT_BODY_BUF_SIZE;
    buf* new_buffers;
    frame_body* _body = &(_frame->body);

    if (_body->buffers != NULL) {
        // realloc if buffers is not null, realloc will copy all data from old pointer to new
        new_buffers = realloc(_body->buffers, alloc_size);
        if (new_buffers == NULL) {
            errorf("cannot realloc frame.body.buffers with size (%d bytes)", alloc_size);
            return s;
        }
    } else {
        // malloc new memory location for buffers
        new_buffers = malloc(alloc_size);
        if (new_buffers == NULL) {
            errorf("cannot malloc frame.body.buffers with size (%d bytes)", alloc_size);
            return s;
        }
        _body->cur_len = 0;
        // should we?
        _body->len = 0;
    }

    _body->buffers = new_buffers;
    _body->buffer_size = alloc_size;

    return s;
}

status _realloc_body_if_need(frame* _frame, uint32_t body_size) {
    status s = STATUS_OK;

    frame_body* _body = &(_frame->body);

    if ((body_size + _body->cur_len) > _body->buffer_size) {
        s = _realloc_body(_frame, body_size);  // all = 1

        if (s != STATUS_OK) {
            return s;
        }
    }

    return s;
}

status _flush_frame_body(frame* _frame) {
    status s = STATUS_OK;

    frame_body* _body = &(_frame->body);

    _body->cur_len = 0;

    return s;
}

void _encode_frame_header(frame* _frame, buf* buffers) {
    // this function assume _frame is not null and buffers has at least frame->header_size

    buf_e32(_frame->size, buffers);
    buf_e16(_frame->op, buffers + 4);
    buf_e8(_frame->off, buffers + 6);
    buf_e8(_frame->flag, buffers + 7);

    for (uint8_t i = 0; i < _frame->off - 2; i++) {
        ext _ext = *(_frame->exts + i);

        for (uint8_t j = 0; j < EXTENSION_SIZE; j++) {
            *(buffers + i * EXTENSION_SIZE + j + BASE_HEADER_SIZE) = *(_ext + j);
        }
    }
}

uint8_t _is_frame_end(frame* _frame) {
    // check header size vs sended size body

    return (_frame->body).len == get_frame_body_size(_frame);
}

status put_frame_body(frame* _frame, buf* buffers, uint32_t buf_size) {
    status s = STATUS_OK;

    if (buf_size <= 0) {
        return s;
    }

    s = validate_args(_frame, buffers);

    if (s != STATUS_OK) {
        return s;
    }

    s = _realloc_body_if_need(_frame, buf_size);

    if (s != STATUS_OK) {
        return s;
    }

    frame_body* _body = &(_frame->body);

    for (uint32_t i = 0; i < buf_size; ++i) {
        *(_body->buffers + i + _body->cur_len) = *(buffers + i);
    }

    /* _body->len += buf_size; */
    _body->cur_len = buf_size;

    return s;
}

status empty_frame(frame* _frame) {
    status s = STATUS_OK;

    s = validate_frame(_frame);

    if (s != STATUS_OK) {
        return s;
    }

    _free_exts_hdr(_frame);
    // for reuse allocated body space, we don't need to free_body, just set len to 0
    /* _free_body(_frame); */
    frame_body* _body = &(_frame->body);
    _body->cur_len = 0;
    _body->len = 0;

    _frame->size = 0;
    _frame->op = 0;
    _frame->off = 2;
    _frame->flag = 0;

    return s;
}

status new_base_frame(frame* _frame, uint16_t _op, uint8_t _flag, buf* data, uint32_t data_size) {
    status s = STATUS_OK;

    s = empty_frame(_frame);

    if (s != STATUS_OK) {
        return s;
    }

    _frame->op = _op;
    _frame->off = 2;
    _frame->flag = _flag;

    s = put_frame_body(_frame, data, data_size);

    if (s != STATUS_OK) {
        return s;
    }

    s = calc_frame_size(_frame, data_size);

    if (s != STATUS_OK) {
        return s;
    }

    return s;
}

void print_frame(frame* _frame) {
    infof("- frame -");
    if (_frame == NULL) {
        infof("null");
    } else {
        infof("# header(size,op,off,flag,body_size)=(%u,%u,%u,%u,%u)", _frame->size, _frame->op, _frame->off,
              _frame->flag, get_frame_body_size(_frame));
        infof("# body(len,cur_len,buf_size)=(%u,%u,%u): %s", (_frame->body).len, (_frame->body).cur_len,
              (_frame->body).buffer_size, _frame->body.buffers);
    }
}

status free_inner_frame(frame* _frame) {
    status s = STATUS_OK;

    if (_frame == NULL) {
        return s;
    }

    s = _free_body(_frame);
    s = _free_exts_hdr(_frame);

    return s;
}

status calc_frame_size(frame* _frame, uint32_t body_size) {
    status s = STATUS_OK;

    s = validate_frame(_frame);

    if (s != STATUS_OK) {
        return s;
    }

    if ((MAX_FRAME_SIZE - body_size) < get_frame_header_size(_frame)) {
        errorf("maximum frame_body size");
        return STATUS_ERR;
    }

    _frame->size = body_size + get_frame_header_size(_frame);

    return s;
}

// parse base frame header
status parse_b_frame_hdr(frame* _frame, buf* buffers, uint32_t buf_size) {
    status s = STATUS_OK;

    s = validate_args(_frame, buffers);
    if (s != STATUS_OK) {
        return s;
    }

    if (buf_size < BASE_HEADER_SIZE) {
        errorf("minimum frame_header is 8 bytes, so buf_size must >= 8");
        return STATUS_ERR;
    }

    // decode size field -> 4 bytes
    _frame->size = buf_d32(buffers);

    // decode next opcode field -> 2 bytes
    _frame->op = buf_d16(buffers + 4);

    // decode next offset field -> 1 bytes
    _frame->off = buf_d8(buffers + 6);
    if (_frame->off < 2) {
        errorf("invalid offset in frame, received %d", _frame->off);
        return s;
    }

    // decode next flag field -> 1 bytes
    _frame->flag = buf_d8(buffers + 7);

    return s;
}

// parse extension frame header
status parse_e_frame_hdr(frame* _frame, buf* buffers, uint32_t buf_size) {
    status s = STATUS_OK;

    s = validate_args(_frame, buffers);

    if (s != STATUS_OK) {
        return STATUS_ERR;
    }

    // allocate new frame extensions
    s = _alloc_exts_hdr(_frame);
    if (s != STATUS_OK) {
        errorf("cannot alloc new extensions field for frame");
        return s;
    }

    if (buf_size < get_frame_exts_hdr_size(_frame)) {
        errorf(
            "buffers does not contain enought bytes for construct extension header,\
            require (%d bytes), received(%d bytes)",
            get_frame_exts_hdr_size(_frame), buf_size);
        return STATUS_ERR;
    }

    for (uint8_t i = 0; i < _frame->off - 2; i++) {
        ext _ext = *(_frame->exts + i);

        for (uint8_t j = 0; j < EXTENSION_SIZE; j++) {
            *(_ext + j) = *(buffers + i * EXTENSION_SIZE + j);
        }
    }

    return s;
}

status read_frame_header(int sockfd, frame* _frame) {
    status s = STATUS_OK;

    uint32_t n_bytes = 0;

    buf buffers[MAX_HEADER_SIZE];

    // read at least to 8 bytes for base headers
    while (n_bytes < BASE_HEADER_SIZE) {
        int _n_bytes = read(sockfd, buffers, BASE_HEADER_SIZE - n_bytes);

        if (_n_bytes <= 0) {
            errorf("read base header error");
            return STATUS_ERR;
        }

        n_bytes += (uint32_t)_n_bytes;
    }

    // let's base extension header
    s = parse_b_frame_hdr(_frame, buffers, n_bytes);
    if (s != STATUS_OK) {
        return s;
    }

    // calculate header size
    uint32_t header_size = get_frame_header_size(_frame);

    // read more bytes for extension header
    while (header_size > n_bytes) {
        int _n_bytes = read(sockfd, buffers, header_size - n_bytes);

        if (_n_bytes <= 0) {
            errorf("read extension header error");
            return STATUS_ERR;
        }

        n_bytes += (uint32_t)_n_bytes;
    }

    // let's parse extension header
    s = parse_e_frame_hdr(_frame, buffers + BASE_HEADER_SIZE, n_bytes - BASE_HEADER_SIZE);
    if (s != STATUS_OK) {
        return s;
    }

    return s;
}

status read_frame_body(int sockfd, frame* _frame, uint32_t data_size, uint32_t* readed_size) {
    status s = STATUS_OK;

    frame_body* _body = &(_frame->body);
    uint32_t body_size_left = get_frame_body_size(_frame) - _body->len;
    // if data_size = 0, we will call one read to read data left in frame
    // we need this behavior to speed up read stream, because we don't want read will block until read all data
    uint32_t body_need_read = data_size > 0 && data_size < body_size_left ? data_size : body_size_left;
    debugf("body_size_left: %u", body_size_left);
    debugf("body_need_read: %u", body_need_read);

    if (body_need_read == 0) {
        return s;
    }

    if (body_need_read < 0) {
        errorf("invalid frame, body_need_read < 0");
        return STATUS_ERR;
    }

    s = _flush_frame_body(_frame);

    if (s != STATUS_OK) {
        return s;
    }

    s = _realloc_body_if_need(_frame, body_need_read);

    if (s != STATUS_OK) {
        return s;
    }

    uint32_t n_bytes = 0;

    while (n_bytes < body_need_read) {
        int _n_bytes = read(sockfd, _body->buffers + n_bytes, body_need_read - n_bytes);

        if (_n_bytes <= 0) {
            errorf("read frame body error");
            return STATUS_ERR;
        }

        n_bytes += (uint32_t)_n_bytes;

        // read just once
        if (data_size == 0) {
            break;
        }
    }

    _body->cur_len = n_bytes;
    _body->len += n_bytes;
    *readed_size = n_bytes;

    return s;
}

status rstream_start(int sockfd, frame* _frame) {
    status s = STATUS_OK;

    s = empty_frame(_frame);

    if (s != STATUS_OK) {
        return s;
    }

    s = read_frame_header(sockfd, _frame);

    if (s != STATUS_OK) {
        return s;
    }

    return s;
}

status rstream_read(int sockfd, frame* _frame, uint32_t data_size, uint32_t* readed_size, uint8_t* next) {
    status s = STATUS_OK;

    s = validate_frame(_frame);

    if (s != STATUS_OK) {
        return s;
    }

    if (get_frame_body_size(_frame) == 0) {
        return s;
    }

    s = read_frame_body(sockfd, _frame, data_size, readed_size);

    if (s != STATUS_OK) {
        return s;
    }

    *next = _is_frame_end(_frame);

    return s;
}

status rstream_end(int sockfd, frame* _frame) {
    status s = STATUS_OK;

    s = validate_frame(_frame);

    if (s != STATUS_OK) {
        return s;
    }

    frame_body* _body = &(_frame->body);
    debugf("body->len: %u", _body->len);

    // if frame is not read all data -> we need read all data left in frame
    if (_body->len < get_frame_body_size(_frame)) {
        uint32_t readed_size = 0;
        uint32_t need_read = get_frame_body_size(_frame) - _body->len;

        s = read_frame_body(sockfd, _frame, need_read, &readed_size);

        if (readed_size < need_read) {
            s = STATUS_ERR;
        }

        if (s != STATUS_OK) {
            return s;
        }
    }

    // validate frame

    if (!_is_frame_end(_frame)) {
        errorf("frame is not end, sended body size is not equal body size in header");
        return STATUS_ERR;
    }

    return s;
}

status read_frame(int sockfd, frame* _frame) {
    status s = STATUS_OK;

    s = rstream_start(sockfd, _frame);

    if (s != STATUS_OK) {
        return s;
    }

    print_frame(_frame);

    s = rstream_end(sockfd, _frame);

    if (s != STATUS_OK) {
        return s;
    }

    return s;
}

status write_frame_header(int sockfd, frame* _frame) {
    status s = STATUS_OK;

    uint32_t header_size = get_frame_header_size(_frame);

    buf buffers[MAX_HEADER_SIZE];

    _encode_frame_header(_frame, buffers);

    uint32_t n_bytes = 0;

    while (n_bytes < header_size) {
        int _n_bytes = write(sockfd, buffers + n_bytes, header_size - n_bytes);

        if (_n_bytes <= 0) {
            errorf("read frame body error");
            return STATUS_ERR;
        }

        n_bytes += (uint32_t)_n_bytes;
    }

    return s;
}

status write_frame_body(int sockfd, frame* _frame) {
    status s = STATUS_OK;

    frame_body* _body = &(_frame->body);

    if ((_body->len + _body->cur_len) > _frame->size) {
        errorf("frame body is larger than frame size.");
        return STATUS_ERR;
    }

    uint32_t n_bytes = 0;

    while (n_bytes < _body->cur_len) {
        int _n_bytes = write(sockfd, _body->buffers + n_bytes, _body->cur_len - n_bytes);

        if (_n_bytes <= 0) {
            errorf("read frame body error");
            return STATUS_ERR;
        }

        n_bytes += (uint32_t)_n_bytes;
    }

    _flush_frame_body(_frame);
    _body->len += n_bytes;

    return s;
}

status wstream_start(int sockfd, frame* _frame) {
    status s = STATUS_OK;

    s = validate_frame(_frame);

    if (s != STATUS_OK) {
        return s;
    }

    s = write_frame_header(sockfd, _frame);

    if (s != STATUS_OK) {
        return s;
    }

    return s;
}

status wstream_write(int sockfd, frame* _frame, buf* data, uint32_t data_size, uint8_t* next) {
    status s = STATUS_OK;

    s = validate_frame(_frame);

    if (s != STATUS_OK) {
        return s;
    }

    if (data != NULL && data_size > 0) {
        s = put_frame_body(_frame, data, data_size);

        if (s != STATUS_OK) {
            return s;
        }
    }

    s = write_frame_body(sockfd, _frame);

    if (s != STATUS_OK) {
        return s;
    }

    *next = _is_frame_end(_frame);

    return s;
}

status wstream_end(int sockfd, frame* _frame) {
    status s = STATUS_OK;

    s = validate_frame(_frame);

    if (s != STATUS_OK) {
        return s;
    }

    frame_body* _body = &(_frame->body);

    // write all buffer left on body if has
    if (_body->cur_len > 0) {
        s = write_frame_body(sockfd, _frame);

        if (s != STATUS_OK) {
            return s;
        }
    }

    // validate frame
    if (!_is_frame_end(_frame)) {
        errorf("frame is not end, sended body size is not equal body size in header");
        return STATUS_ERR;
    }

    return s;
}

status write_frame(int sockfd, frame* _frame) {
    status s = STATUS_OK;

    /* s = wstream_start(sockfd, _frame, (_frame->body).cur_len); */
    s = wstream_start(sockfd, _frame);

    if (s != STATUS_OK) {
        return s;
    }

    s = wstream_end(sockfd, _frame);

    if (s != STATUS_OK) {
        return s;
    }

    return s;
}
