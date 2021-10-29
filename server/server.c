/**
 * Dao Minh Hai - 18020445
 * server.c:
 * Basic file transfer protocol.
 *
 */
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../shared/clog.h"
#include "../shared/fileio.h"
#include "../shared/frame.h"

#define DEFAULT_PORT 29010
#define DEFAULT_ADDR "127.0.0.1"
#define MAX_BUFFER 1024

int pexit(const char* str) {
    perror(str);
    exit(1);
}

int create_serv_socket(struct sockaddr_in* serv_sockaddr) {
    // create new socketfd
    int serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    socklen_t serv_sockaddr_len = sizeof(*serv_sockaddr);

    if (serv_sockfd < 0) {
        pexit("socket()");
    }

    infof("Start binding server socket");
    if (bind(serv_sockfd, (struct sockaddr*)serv_sockaddr, serv_sockaddr_len) < 0) {
        pexit("bind()");
    }

    infof("Start listen server socket");
    if (listen(serv_sockfd, 10) < 0) {
        pexit("listen()");
    }

    infof("Server listening on %s:%d", inet_ntoa(serv_sockaddr->sin_addr), ntohs(serv_sockaddr->sin_port));

    return serv_sockfd;
}

status _op_hello(int sockfd, frame* req, frame* res) {
    status s = STATUS_OK;

    infof("received 'HELLO' frame");

    char msg[] = "HELLO OK\r\n";
    s = new_base_frame(res, OP_HELLO, FL_SUCCESS, msg, strlen(msg));

    if (s != STATUS_OK) {
        return s;
    }

    write_frame(sockfd, res);

    return s;
}

status _op_get_file(int sockfd, frame* req, frame* res) {
    status s = STATUS_OK;

    infof("received 'GET_FILE' frame");

    file_io _fio = {};

    s = fileio_new(&_fio, (req->body).buffers, (req->body).len);
    /* s = buf_to_fileio(&_fio, (req->body).buffers, (req->body).len); */

    if (s != STATUS_OK) {
        return s;
    }

    s = fileio_open(&_fio, "rb");
    if (s != STATUS_OK) {
        infof("send 'FILE NOT FOUND' frame");
        char msg[] = "FILE NOT FOUND\r\n";
        s = new_base_frame(res, OP_GET_FILE, FL_GF_NOTFOUND, msg, strlen(msg));

        write_frame(sockfd, res);

        s = fileio_close(&_fio);

        return s;
    }

    // send file info
    buf buffers[259];
    uint32_t buf_size = 0;

    s = fileio_to_buf(&_fio, buffers, &buf_size);

    if (s != STATUS_OK) {
        fileio_close(&_fio);
        return s;
    }

    s = new_base_frame(res, OP_GET_FILE, FL_GF_INFO, buffers, buf_size);

    if (s != STATUS_OK) {
        fileio_close(&_fio);
        return s;
    }

    /* debugf("send info"); */
    infof("send 'FILE INFO' frame");
    s = write_frame(sockfd, res);

    if (s != STATUS_OK) {
        fileio_close(&_fio);
        return s;
    }

    // send file data

    infof("start send 'FILE DATA' frame");
    s = new_empty_body_frame(res, OP_GET_FILE, FL_GF_DATA, _fio.size);

    if (s != STATUS_OK) {
        fileio_close(&_fio);
        return s;
    }

    /* debugf("start send data"); */
    s = wstream_start(sockfd, res);
    /* debugf("done start"); */

    if (s != STATUS_OK) {
        fileio_close(&_fio);
        return s;
    }

    uint8_t next = 1;

    frame_body* res_body = &(res->body);

    while (next) {
        s = fileio_read(&_fio, res_body->buffers, res_body->buffer_size, &(res_body->cur_len), &next);

        if (s != STATUS_OK) {
            fileio_close(&_fio);
            return s;
        }

        uint8_t stream_next = 1;

        /* debugf("write call %d, %d, %d", res_body->cur_len, res_body->len, res->size); */
        s = wstream_write(sockfd, res, NULL, 0, &stream_next);
        /* debugf("write called"); */
        if (s != STATUS_OK) {
            fileio_close(&_fio);
            return s;
        }

        /* next = 0; */
        if (stream_next != next || s != STATUS_OK) {
            fileio_close(&_fio);
            return STATUS_ERR;
        }
    }

    /* debugf("end send data"); */
    wstream_end(sockfd, res);
    fileio_close(&_fio);

    return s;
}

status _op_quit(int sockfd, frame* req, frame* res) {
    status s = STATUS_OK;

    infof("received 'QUIT' frame");

    char msg[] = "QUIT OK\r\n";
    s = new_base_frame(res, OP_QUIT, FL_SUCCESS, msg, strlen(msg));

    if (s != STATUS_OK) {
        return s;
    }

    write_frame(sockfd, res);

    if (s != STATUS_OK) {
        return s;
    }

    return STATUS_CLOSE;
}

status _op_invalid(int sockfd, frame* req, frame* res) {
    status s = STATUS_OK;

    infof("received 'INVALID' frame");

    char msg[] = "INVALID OPCODE\r\n";
    s = new_base_frame(res, OP_INVALID, FL_SUCCESS, msg, strlen(msg));

    if (s != STATUS_OK) {
        return s;
    }

    s = write_frame(sockfd, res);

    return s;
}

status handle_frame(int sockfd, frame* req, frame* res) {
    status s = STATUS_OK;

    // big switch :)
    // @todo split into smaller function
    switch (req->op) {
        case OP_HELLO:
            s = _op_hello(sockfd, req, res);
            break;
        case OP_GET_FILE:
            s = _op_get_file(sockfd, req, res);
            break;
        case OP_QUIT:
            s = _op_quit(sockfd, req, res);
            break;
        case OP_INVALID:
            s = _op_invalid(sockfd, req, res);
            break;
        default:
            s = _op_invalid(sockfd, req, res);
    }

    return s;
}

status accept_frame(const int cli_sockfd) {
    status s = STATUS_OK;

    frame frame_req = {};
    frame frame_res = {};

    while (1) {
        s = read_frame(cli_sockfd, &frame_req);

        if (s != STATUS_OK) {
            perror("read_frame()");
            return s;
        }

        debugf("received frame ok ->");

        print_frame(&frame_req);

        s = handle_frame(cli_sockfd, &frame_req, &frame_res);

        debugf("handle frame ok ->");

        if (s != STATUS_OK) {
            perror("handle_frame()");
            return s;
        }
    }
}

void accept_n_serve_request(const int serv_sockfd) {
    // infinite loop accepting new connection and serving request
    while (1) {
        // new sockaddr for client
        struct sockaddr_in cli_sockaddr;
        // fill all zero to cli_sockaddr, prefer using memset than bzero
        memset(&cli_sockaddr, 0, sizeof(cli_sockaddr));
        /* bzero(&cli_sockaddr, sizeof(cli_sockaddr)); */
        socklen_t cli_sockaddr_len = sizeof(cli_sockaddr);
        /* socklen_t cli_sockaddr_len; */

        // accept new connection, return new socketfd
        int cli_sockfd = accept(serv_sockfd, (struct sockaddr*)&cli_sockaddr, &cli_sockaddr_len);
        if (cli_sockfd < 0) {
            pexit("accept()");
        }

        infof("Received connection from %s:%d", inet_ntoa(cli_sockaddr.sin_addr), ntohs(cli_sockaddr.sin_port));

        // start handle request from this connection
        accept_frame(cli_sockfd);

        /* sleep(1); */
        // close client connection
        close(cli_sockfd);
        perror("close()");

        infof("Closed connection from %s:%d", inet_ntoa(cli_sockaddr.sin_addr), ntohs(cli_sockaddr.sin_port));
    }
}

int main(int argc, char* argv[]) {
    // declare socket attribute
    int listen_port = DEFAULT_PORT;
    const char* listen_addr = DEFAULT_ADDR;

    // parsing command line arguments
    if (argc > 1) {
        // parsing port_number at args[1]
        listen_port = atoi(argv[1]);
    }
    if (argc > 2) {
        listen_addr = argv[2];
    }

    // construct server_sockaddr
    struct sockaddr_in serv_sockaddr;
    memset(&serv_sockaddr, 0, sizeof(serv_sockaddr));
    /* bzero(&serv_sockaddr, sizeof(serv_sockaddr)); */
    serv_sockaddr.sin_family = AF_INET;
    serv_sockaddr.sin_addr.s_addr = inet_addr(listen_addr);
    serv_sockaddr.sin_port = htons(listen_port);

    // create sockfd, listening, binding in serv_sockaddr
    int serv_sockfd = create_serv_socket(&serv_sockaddr);

    // accept new request and handle request
    accept_n_serve_request(serv_sockfd);

    // dont forget to close your server
    close(serv_sockfd);

    return 0;
}
