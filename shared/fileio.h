#ifndef _FILEIO_H
#define _FILEIO_H

#include <stdint.h>
#include <stdio.h>

#include "frame.h"

#define MAX_FILE_NAME 255

typedef struct {
    FILE* fp;
    uint32_t size;
    char name[MAX_FILE_NAME + 1];
} file_io;

status fileio_to_buf(file_io* fio, buf* buffers, uint32_t* buf_size);
status buf_to_fileio(file_io* fio, buf* buffers, uint32_t buf_size);
status fileio_new(file_io* fio, char* name, uint32_t len);
status fileio_open(file_io* fio, char* mode);
status fileio_close(file_io* fio);
status fileio_read(file_io* fio, buf* buffer, uint32_t buffer_size, uint32_t* read_size, uint8_t* next);
status fileio_write(file_io* fio, buf* buffer, uint32_t buffer_size);

#endif
