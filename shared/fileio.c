#include "fileio.h"

#include <stdio.h>
#include <string.h>

#include "clog.h"
#include "frame.h"

status fileio_to_buf(file_io* fio, buf* buffers, uint32_t* buf_size) {
    status s = STATUS_OK;

    if (fio == NULL || buffers == NULL) {
        return STATUS_ERR;
    }

    buf_e32(fio->size, buffers);
    memcpy(buffers + 4, fio->name, strlen(fio->name));

    *buf_size = 4 + strlen(fio->name);

    return s;
}

status buf_to_fileio(file_io* fio, buf* buffers, uint32_t buf_size) {
    status s = STATUS_OK;

    if (fio == NULL || buffers == NULL) {
        return STATUS_ERR;
    }

    s = fileio_close(fio);

    if (s != STATUS_OK) {
        return s;
    }

    fio->size = buf_d32(buffers);
    memcpy(fio->name, buffers + 4, buf_size - 4);
    /* fio->name[buf_size - 4] = '_'; */
    fio->name[buf_size - 4] = '\0';

    return s;
}

status fileio_close(file_io* fio) {
    status s = STATUS_OK;

    if (fio == NULL || fio->fp == NULL) {
        return s;
    }

    fclose(fio->fp);
    /* fio->size = 0; */
    /* *(fio->name) = '\0'; */
    infof("fileio_close");

    return s;
}

status fileio_open(file_io* fio, char* mode) {
    status s = STATUS_OK;

    if (fio == NULL || !strlen(fio->name)) {
        return STATUS_ERR;
    }

    s = fileio_close(fio);

    if (s != STATUS_OK) {
        return s;
    }

    /* debugf("open_file: %s", fio->name); */

    FILE* fi = fopen(fio->name, mode);
    if (fi == NULL) {
        errorf("fopen error.");
        perror("fopen()");
        return STATUS_ERR;
    }

    fio->fp = fi;

    // get file size
    fseek(fi, 0, SEEK_END);

    fio->size = ftell(fi);
    debugf("file: %s (%ld bytes)", fio->name, ftell(fi));

    // return back to start of file
    fseek(fi, 0, SEEK_SET);

    return s;
}

status fileio_new(file_io* fio, char* name, uint32_t len) {
    status s = STATUS_OK;

    if (fio == NULL) {
        return STATUS_ERR;
    }

    fileio_close(fio);

    memcpy(fio->name, name, len);
    *(fio->name + len) = '\0';

    return s;
}

status fileio_read(file_io* fio, buf* buffer, uint32_t buffer_size, uint32_t* read_size, uint8_t* next) {
    status s = STATUS_OK;

    if (fio == NULL) {
        return STATUS_ERR;
    }

    int n_bytes = fread(buffer, sizeof(buf), buffer_size, fio->fp);

    *next = !feof(fio->fp);

    if (n_bytes < buffer_size && ferror(fio->fp)) {
        errorf("fread error.");
        perror("fread()");
        return STATUS_ERR;
    }

    *read_size = (uint32_t)n_bytes;
    /* debugf("file io read: %d", n_bytes); */

    return s;
}

status fileio_write(file_io* fio, buf* buffer, uint32_t buffer_size) {
    status s = STATUS_OK;

    if (fio == NULL) {
        return STATUS_ERR;
    }

    if (fio->fp == NULL) {
        return STATUS_ERR;
    }

    /* debugf("io write: %u %u", buffer_size, sizeof(buf)); */
    /* debugf("io write buffer %p %s", buffer, buffer); */

    int n_bytes = fwrite(buffer, sizeof(buf), buffer_size, fio->fp);

    /* debugf("done io write: %u", buffer_size); */

    if (n_bytes < buffer_size && ferror(fio->fp)) {
        errorf("fwrite error.");
        perror("fwrite()");
        return STATUS_ERR;
    }

    return s;
}
