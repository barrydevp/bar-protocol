/**
 * Dao Minh Hai - 18020445
 * client.c:
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

#define SERVER_PORT 29010
#define DEFAULT_SERVER_ADDR "127.0.0.1"
#define BUFFER_SIZE 1024
#define QUIT_MSG "QUIT"

int pexit(const char* str) {
    perror(str);
    exit(1);
}

int connect_server(struct sockaddr_in* serv_sockaddr) {
    infof("Connecting %s:%d ...\n", inet_ntoa(serv_sockaddr->sin_addr), ntohs(serv_sockaddr->sin_port));

    int conn_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (conn_sockfd < 0) {
        pexit("socket()");
    }

    if (connect(conn_sockfd, (struct sockaddr*)serv_sockaddr, sizeof(*serv_sockaddr)) < 0) {
        pexit("connect()");
    }

    infof("Connected %s:%d !\n\tSAY SOMETHING WITH SERVER :>\n", inet_ntoa(serv_sockaddr->sin_addr),
          ntohs(serv_sockaddr->sin_port));
    infof("-------------------------------------------------\n");

    return conn_sockfd;
}

status req_op_hello(frame* req) {
    status s = STATUS_OK;

    infof("request 'HELLO' frame");

    if (s != STATUS_OK) {
        return s;
    }

    char msg[] = "HELLO SERVER\r\n";
    s = new_base_frame(req, OP_HELLO, FL_REQUEST, msg, strlen(msg));

    return s;
}

status req_op_get_file(frame* req, char* filename) {
    status s = STATUS_OK;

    infof("request 'GET_FILE' frame");

    if (s != STATUS_OK) {
        return s;
    }

    s = new_base_frame(req, OP_GET_FILE, FL_REQUEST, filename, strlen(filename));

    return s;
}

status req_op_quit(frame* req) {
    status s = STATUS_OK;

    infof("request 'QUIT' frame");

    if (s != STATUS_OK) {
        return s;
    }

    char msg[] = "QUIT SERVER\r\n";
    s = new_base_frame(req, OP_QUIT, FL_REQUEST, msg, strlen(msg));

    return s;
}

status _op_hello(int sockfd, frame* res) {
    status s = STATUS_OK;

    infof("received 'HELLO' frame");

    return s;
}

status _op_get_file(int sockfd, frame* res) {
    status s = STATUS_OK;

    infof("received 'HELLO' frame");
    uint8_t next = 1;

    file_io rfio = {};
    file_io wfio = {};

    uint32_t n_bytes = 0;

    frame_body* res_body = &(res->body);

    while (next) {
        switch (res->flag) {
            case FL_ERROR:
                return STATUS_ERR;
            case FL_GF_NOTFOUND:
                return s;
            case FL_GF_INFO:
                infof("received 'file info' frame");

                s = buf_to_fileio(&rfio, (res->body).buffers, (res->body).len);

                if (s != STATUS_OK) {
                    next = 0;
                    break;
                }

                sprintf(wfio.name, "down_%s", rfio.name);
                s = fileio_new(&wfio, wfio.name, strlen(wfio.name));
                s = fileio_open(&wfio, "wb");

                if (s != STATUS_OK) {
                    next = 0;
                    break;
                }

                s = rstream_start(sockfd, res);

                if (s != STATUS_OK) {
                    next = 0;
                    break;
                }

            case FL_GF_DATA:
                /* infof("received 'data file' frame"); */

                if (rfio.size < 0) {
                    next = 0;
                    s = STATUS_ERR;
                    errorf("file_size < 0");
                    break;
                }

                if (rfio.size == 0) {
                    next = 0;
                    break;
                }

                s = rstream_read(sockfd, res, 0, &n_bytes, &next);

                /* debugf("rs_read: %u", n_bytes); */

                if (s != STATUS_OK) {
                    next = 0;
                    break;
                }

                if (n_bytes > 0) {
                    s = fileio_write(&wfio, res_body->buffers, res_body->cur_len);

                    if (s != STATUS_OK) {
                        next = 0;
                        break;
                    }
                }

                infof("Downloading %u / %u bytes", res_body->len, rfio.size);

                break;
            default:
                next = 0;
                s = STATUS_ERR;
                break;
        }
    }

    if (s == STATUS_OK) {
        infof("Download done -> your file: %s", wfio.name);
    }

    fileio_close(&rfio);
    fileio_close(&wfio);

    return s;
}

status _op_quit(int sockfd, frame* res) {
    status s = STATUS_CLOSE;

    infof("received 'QUIT' frame");

    return s;
}

status _op_invalid(int sockfd, frame* res) {
    status s = STATUS_OK;

    infof("received 'INVALID' frame");

    return s;
}

void remove_line_feed(char* input) {
    // replace new line with null character '\0'
    char* last_ch = strrchr(input, '\n');
    if (last_ch == NULL) {
        return;
    }
    *last_ch = '\0';
}

uint16_t input_to_opcode(char* input) {
    if (strcasecmp(input, "HELLO") == 0) {
        return OP_HELLO;
    }

    if (strcasecmp(input, "QUIT") == 0) {
        return OP_QUIT;
    }

    if (strcasecmp(input, "GET_FILE") == 0) {
        return OP_GET_FILE;
    }

    return OP_INVALID;
}

status make_request(int sockfd, frame* req) {
    status s = STATUS_OK;

    char buffers[BUFFER_SIZE];

    uint8_t input_ok = 1;

    do {
        printf("Opcode (hello) - (get_file) - (quit): ");
        // read line from stdin (screen)
        if (fgets(buffers, BUFFER_SIZE, stdin) == NULL) {
            errorf("Cannot read from stdin");

            return STATUS_ERR;
        }

        remove_line_feed(buffers);

        input_ok = 1;

        // big switch :)
        // @todo split into smaller function
        switch (input_to_opcode(buffers)) {
            case OP_HELLO:
                s = req_op_hello(req);
                break;
            case OP_GET_FILE:
                printf("file_name: ");
                if (fgets(buffers, BUFFER_SIZE, stdin) == NULL) {
                    errorf("Cannot read from stdin");

                    return STATUS_ERR;
                }
                remove_line_feed(buffers);

                s = req_op_get_file(req, buffers);
                break;
            case OP_QUIT:
                s = req_op_quit(req);
                break;
            default:
                errorf("Invalid opcode, retry");
                input_ok = 0;
        }
    } while (!input_ok);

    print_frame(req);

    s = write_frame(sockfd, req);

    return s;
}

status handle_frame(int sockfd, frame* res) {
    status s = STATUS_OK;

    // big switch :)
    // @todo split into smaller function
    switch (res->op) {
        case OP_HELLO:
            s = _op_hello(sockfd, res);
            break;
        case OP_GET_FILE:
            s = _op_get_file(sockfd, res);
            break;
        case OP_QUIT:
            s = _op_quit(sockfd, res);
            break;
        case OP_INVALID:
            s = _op_invalid(sockfd, res);
            break;
        default:
            s = _op_invalid(sockfd, res);
    }

    return s;
}

status request_reply(const int conn_sockfd) {
    status s = STATUS_OK;

    frame frame_req = {};
    frame frame_res = {};

    while (1) {
        s = make_request(conn_sockfd, &frame_req);

        if (s != STATUS_OK) {
            perror("make_request()");
            return s;
        }

        debugf("send frame ok ->");

        s = read_frame(conn_sockfd, &frame_res);

        if (s != STATUS_OK) {
            perror("read_frame()");
            return s;
        }

        debugf("received frame ok ->");

        print_frame(&frame_res);

        s = handle_frame(conn_sockfd, &frame_res);

        debugf("handle frame ok ->");

        if (s != STATUS_OK) {
            perror("handle_frame()");
            return s;
        }
    }

    return s;
}

int main(int argc, char** argv) {
    int server_port = SERVER_PORT;
    const char* server_addr = DEFAULT_SERVER_ADDR;

    // parsing command line arguments
    if (argc > 1) {
        // parsing port_number at args[1]
        server_port = atoi(argv[1]);
    }
    if (argc > 2) {
        server_addr = argv[2];
    }

    struct sockaddr_in serv_sockaddr;
    memset(&serv_sockaddr, 0, sizeof(serv_sockaddr));
    /* bzero(&serv_sockaddr, sizeof(serv_sockaddr)); */
    serv_sockaddr.sin_family = AF_INET;
    serv_sockaddr.sin_addr.s_addr = inet_addr(server_addr);
    serv_sockaddr.sin_port = htons(server_port);

    int conn_sockfd = connect_server(&serv_sockaddr);

    request_reply(conn_sockfd);

    /* sleep(1); */
    // close the connection
    close(conn_sockfd);
    perror("close()");
    printf("Closed app.\n");

    return 0;
}
