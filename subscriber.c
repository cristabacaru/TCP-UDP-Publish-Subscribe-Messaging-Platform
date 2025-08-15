#include<string.h>
#include <sys/epoll.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "common.h"
#include <math.h>

#define MAX_CONNECTIONS 2

// data type names used for printing
const char DATA_TYPES[4][12] = {
    "INT",
    "SHORT_REAL",
    "FLOAT",
    "STRING"
};

// function to handle user input
int parse_input (char id[11], int listenfd) {
    struct tcp_message mes;
    strcpy(mes.id, id);
    strcpy(mes.command, "");
    strcpy(mes.message, "");

    char buf[100];

    if (fgets(buf, sizeof(buf), stdin)) {
        int i = 0;
        char *p = strtok(buf, " ");

        // parse command and message
        while(p) {
            if (i == 0) {
                strcpy(mes.command, p);  // first token is the command
                i++;
            } else {
                strcpy(mes.message, p);  // second token is the topic
                mes.message[strlen(mes.message) - 1] = '\0';  // remove newline
            }
            p = strtok(NULL, " ");
        }

        send_all(listenfd, &mes, sizeof(mes));  // send the command to server

        if (strstr(mes.command, "exit") != 0) {
            mes.command[strlen(mes.command) - 1] = '\0';
            return 1;  // exit signal
        }

        if (strcmp(mes.command, "subscribe") == 0) {
            printf("Subscribed to topic %s\n", mes.message);
            return 0;
        }

        if (strcmp(mes.command, "unsubscribe") == 0) {
            printf("Unsubscribed from topic %s\n", mes.message);
            return 0;
        }
    } else {
        return 0;
    }

    return 0;
}

// function to print out the UDP message nicely
void print_message (char ip[16], int port, char topic[51], int data_type, char message[1500]) {
    printf("%s:%d - %s - %s - ", ip, port, topic, DATA_TYPES[data_type]);

    if (data_type == 0) {
        // INT
        int sign = message[0];
        uint32_t value = 0;
        memcpy(&value, message + 1, sizeof(uint32_t));
        if (sign && value != 0) {
            printf("-%d\n", ntohl(value));
        } else {
            printf("%d\n", ntohl(value));
        }
        return;
    }

    if (data_type == 1) {
        // SHORT_REAL
        uint16_t value = 0;
        memcpy(&value, message, sizeof(uint16_t));
        value = ntohs(value);
        printf("%.2f\n", value / 100.0);
        return;
    }

    if (data_type == 2) {
        // FLOAT
        int sign = message[0];
        uint32_t value = 0;
        uint8_t powIndex = 0;
        memcpy(&value, message + 1, sizeof(uint32_t));
        memcpy(&powIndex, message + 5, sizeof(uint8_t));
        value = ntohl(value);
        double result = value / pow(10, powIndex);
        if (sign && value != 0) {
            printf("-%.*g\n", 10, result);
        } else {
            printf("%.*g\n", 10, result);
        }
        return;
    }

    if (data_type == 3) {
        // STRING
        printf("%s\n", message);
    }
}

// main loop that listens for messages or user input
void run_client(int listenfd, char id[11]) {
    int epollfd = epoll_create1(0);
    if (epollfd < 0) {
        fprintf(stderr, "Failed to create epoll\n");
        exit(1);
    }

    struct epoll_event events[2];
    struct epoll_event ev;

    // socket for messages from server
    ev.events = EPOLLIN;
    ev.data.fd = listenfd;
    int rc = epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);
    if (rc < 0) {
        fprintf(stderr, "Failed to add socket to epoll\n");
        exit(1);
    }

    // socket for keyboard input
    ev.events = EPOLLIN;
    ev.data.fd = STDIN_FILENO;
    rc = epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);
    if (rc < 0) {
        fprintf(stderr, "Failed to add stdin to epoll\n");
        exit(1);
    }

    while(1) {
        int current_events = epoll_wait(epollfd, events, 2, -1);
        if (current_events < 0) {
            fprintf(stderr, "Issue with waiting for events\n");
            exit(1);
        }

        for (int i = 0; i < current_events; i++) {
            if (events[i].events & EPOLLIN) {
                if (events[i].data.fd == listenfd) {
                    // got message from server
                    struct udp_message mes;
                    int rc = recv_all(events[i].data.fd, &mes, sizeof(mes));

                    if (rc <= 0) {
                        // server closed connection
                        close(epollfd);
                        return;
                    }

                    // print message
                    print_message(mes.ip, mes.port, mes.topic, mes.data_type, mes.message);

                } else if (events[i].data.fd == STDIN_FILENO) {
                    // user command input
                    int rc = parse_input(id, listenfd);
                    if (rc == 1) {
                        close(epollfd);
                        return;
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Not enough arguments!\n");
        exit(1);
    }

    // turn off stdout buffering
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    int port = atoi(argv[3]);
    const int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if (listenfd < 0) {
        fprintf(stderr, "Failed to create socket!\n");
        exit(1);
    }

    // setup server address
    struct sockaddr_in TCP_addr;
    memset(&TCP_addr, 0, sizeof(TCP_addr));
    TCP_addr.sin_family = AF_INET;
    TCP_addr.sin_port = htons(port);

    int rc = inet_pton(AF_INET, argv[2], &TCP_addr.sin_addr.s_addr);
    if (rc < 0) {
        fprintf(stderr, "Failed to parse ip address\n");
        exit(1);
    }

    // turn off Nagle’s algorithm
    int option = 1;
    rc = setsockopt(listenfd, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(int));
    if (rc < 0) {
        fprintf(stderr, "Failed to disable Nagle's algorithm\n");
        exit(1);
    }

    // try connecting to server
    rc = connect(listenfd, (struct sockaddr *)&TCP_addr, sizeof(TCP_addr));
    if (rc < 0) {
        fprintf(stderr, "Failed to connect to server\n");
        exit(1);
    }

    // send only the client ID at the beginning
    struct tcp_message mes;
    strcpy(mes.id, argv[1]);
    send_all(listenfd, &mes, sizeof(mes));

    // run the client main loop
    run_client(listenfd, argv[1]);

    close(listenfd);

    return 0;
}
