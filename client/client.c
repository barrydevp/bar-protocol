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

status req_op_hello(frame* res) {
    status s = STATUS_OK;

    infof("request 'HELLO' frame");

    if (s != STATUS_OK) {
        return s;
    }

    char msg[] = "HELLO SERVER\r\n";
    s = new_base_frame(res, OP_HELLO, FL_REQUEST, msg, strlen(msg));

    return s;
}

status req_op_quit(frame* res) {
    status s = STATUS_OK;

    infof("request 'QUIT' frame");

    if (s != STATUS_OK) {
        return s;
    }

    char msg[] = "QUIT SERVER\r\n";
    s = new_base_frame(res, OP_QUIT, FL_REQUEST, msg, strlen(msg));

    return s;
}

status _op_hello(int sockfd, frame* res) {
    status s = STATUS_OK;

    infof("received 'HELLO' frame");

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

uint16_t input_to_opcode(char* input) {
    // replace new line with null character '\0'
    char* last_ch = strrchr(input, '\n');
    if (last_ch == NULL) {
        return OP_INVALID;
    }
    *last_ch = '\0';

    if (strcasecmp(input, "HELLO") == 0) {
        return OP_HELLO;
    }

    if (strcasecmp(input, "QUIT") == 0) {
        return OP_QUIT;
    }

    return OP_INVALID;
}

status make_request(int sockfd, frame* req) {
    status s = STATUS_OK;

    char buffers[BUFFER_SIZE];

    uint8_t input_ok = 1;

    do {
        printf("Opcode: ");
        // read line from stdin (screen)
        if (fgets(buffers, BUFFER_SIZE, stdin) == NULL) {
            errorf("Cannot read from stdin");

            return STATUS_ERR;
        }

        input_ok = 1;

        // big switch :)
        // @todo split into smaller function
        switch (input_to_opcode(buffers)) {
            case OP_HELLO:
                s = req_op_hello(req);
                break;
            /* case OP_GET_FILE: */
            /*     s = _op_hello(req, res); */
            /*     break; */
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
        /* case OP_GET_FILE: */
        /*     s = _op_hello(req, res); */
        /*     break; */
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
