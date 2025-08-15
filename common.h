#include<string.h>
#include <sys/epoll.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h> 
#include <arpa/inet.h>
#include <stdbool.h>


struct __attribute__((packed)) tcp_message{
    char id[11];
    char command[15];
    char message[51];
};

struct __attribute__((packed)) udp_message {
    char ip[16];
    int port;
    char topic[51];
    uint8_t data_type;
    char message[1501];
};

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);
bool find_match(char full_topic[51], char path[51]);