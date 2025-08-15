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
#include "map.h"
#include "common.h"

#define MAX_CONNECTIONS 32
#define UDP_MES_SIZE 1569

int create_tcp_listener(int port) {
    // create TCP socket
    const int listenfd_TCP = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd_TCP < 0) {
        fprintf(stderr, "Failed to create socket!\n");
        exit(1);
    }

    // make socket reusable
    int option = 1;
    int rc;
    rc = setsockopt(listenfd_TCP, SOL_SOCKET, SO_REUSEADDR,
                &option, sizeof(int));
    
    if (rc < 0) {
        fprintf(stderr, "Failed to make socket address reusable\n");
        exit(1);
    }

    // disable Nagle’s algorithm
    rc = setsockopt(listenfd_TCP, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(int));
    if (rc < 0) {
        fprintf(stderr, "Failed to disable Nagle's algorithm\n");
        exit(1);
    }

    // prepare address structure
    struct sockaddr_in TCP_addr;

    memset(&TCP_addr, 0, sizeof(TCP_addr));
    TCP_addr.sin_family = AF_INET;
    TCP_addr.sin_addr.s_addr = INADDR_ANY;
    TCP_addr.sin_port = htons(port);
 
    // bind socket to the address
    rc = bind(listenfd_TCP, (const struct sockaddr *)&TCP_addr, sizeof(TCP_addr));
    if (rc < 0) {
        fprintf(stderr, "Bind failed \n");
        exit(1);
    }

    // listen for incoming connections
    rc = listen(listenfd_TCP, MAX_CONNECTIONS);
    if (rc < 0) {
        fprintf(stderr, "Listen failed\n");
        exit(1);
    }

    return listenfd_TCP;
}

int create_udp_listener(int port) {
    // create UDP socket
    const int listenfd_UDP = socket(AF_INET, SOCK_DGRAM, 0);
    if (listenfd_UDP < 0) {
        fprintf(stderr, "Failed to create socket!\n");
        exit(1);
    }

    // make socket reusable
    int option = 1;
    int rc;
    rc = setsockopt(listenfd_UDP, SOL_SOCKET, SO_REUSEADDR,
        &option, sizeof(int));

    if (rc < 0) {
        fprintf(stderr, "Failed to make socket address reusable\n");
        exit(1);
    }

    // set address details
    struct sockaddr_in UDP_addr;
    memset(&UDP_addr, 0, sizeof(UDP_addr));
    UDP_addr.sin_family = AF_INET;
    UDP_addr.sin_addr.s_addr = INADDR_ANY;
    UDP_addr.sin_port = htons(port);

    // bind the socket to the address
    rc = bind(listenfd_UDP, (const struct sockaddr *)&UDP_addr, sizeof(UDP_addr));
    if (rc < 0) {
        fprintf(stderr, "Bind failed \n");
        exit(1);
    }

    return listenfd_UDP;
}

struct udp_message *create_udp_message (char* buf) {
    // extract data from buffer into custom udp_message struct
    struct udp_message *mes = malloc(sizeof(struct udp_message));
    strncpy(mes->topic, buf, 50); // copy topic (max 50 chars)
    mes->topic[50] = '\0'; // just in case, terminate string
    mes->data_type = *(buf + 50); // extract data type
    memcpy(mes->message, buf + 51, 1500); // copy actual message
    return mes;
}

void run_server(int tcp_listerfd, int udp_listenerfd) {
    int epollfd = epoll_create1(0); // create epoll instance
    if (epollfd < 0) {
        fprintf(stderr, "Failed to creade epoll\n");
        exit(1);
    }

    // set sockets to non-blocking mode
    int flags = fcntl(tcp_listerfd, F_GETFL, 0);
    fcntl(tcp_listerfd, F_SETFL, flags | O_NONBLOCK);

    flags = fcntl(udp_listenerfd, F_GETFL, 0);
    fcntl(udp_listenerfd, F_SETFL, flags | O_NONBLOCK);

    struct epoll_event events[MAX_CONNECTIONS];
    struct epoll_event ev;

    // add TCP listener to epoll
    ev.events = EPOLLIN;
    ev.data.fd = tcp_listerfd;
    int rc = epoll_ctl(epollfd, EPOLL_CTL_ADD, tcp_listerfd, &ev);
    if (rc < 0) {
        fprintf(stderr, "Failed to add socket to epoll\n");
        exit(1);
    }

    // add UDP listener to epoll
    ev.events = EPOLLIN;
    ev.data.fd = udp_listenerfd;
    rc = epoll_ctl(epollfd, EPOLL_CTL_ADD, udp_listenerfd, &ev);
    if (rc < 0) {
        fprintf(stderr, "Failed to add socket to epoll\n");
        exit(1);
    }

    // add stdin to epoll for "exit" command
    ev.events = EPOLLIN;
    ev.data.fd = STDIN_FILENO;
    rc = epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);
    if (rc < 0) {
        fprintf(stderr, "Failed to add socket to epoll\n");
        exit(1);
    }

    // maps to keep track of clients and their topics
    struct mapStruct *subscribers_map = map_init();
    struct mapStruct *subscribers_topics = map_init();

    while(1) {
        // wait for events
        int current_events = epoll_wait(epollfd, events, MAX_CONNECTIONS, -1);
        if (current_events < 0) {
            fprintf(stderr, "Issue with waiting for events\n");
            exit(1);
        }
        for (int i = 0; i < current_events; i++) {
            if (events[i].events & EPOLLIN) {
                // handle new TCP client
                if (events[i].data.fd == tcp_listerfd) {
                    struct sockaddr_in subscriber_addr;
                    socklen_t addrlen = sizeof(subscriber_addr);
                    const int sub_sockfd = accept (tcp_listerfd, 
                        (struct sockaddr *)&subscriber_addr, 
                        &addrlen);
                    
                    struct tcp_message recv_mes;
                    recv_all(sub_sockfd, &recv_mes, sizeof(recv_mes));

                    // check if this client is already connected
                    struct pairStruct* pair = map_get(subscribers_map, recv_mes.id);
                    if (pair != NULL) {
                        printf("Client %s already connected.\n", recv_mes.id);
                        close(sub_sockfd);
                    } else {
                        // new client, add to epoll and map
                        ev.events = EPOLLIN;
                        ev.data.fd = sub_sockfd;
                        rc = epoll_ctl(epollfd, EPOLL_CTL_ADD, sub_sockfd, &ev);
                        if (rc < 0) {
                            fprintf(stderr, "Failed to add socket to epoll\n");
                            exit(1);
                        }
                        printf("New client %s connected from %s:%d.\n", recv_mes.id, 
                            inet_ntoa(subscriber_addr.sin_addr), 
                            ntohs(subscriber_addr.sin_port));
                        map_add_int(subscribers_map, recv_mes.id, sub_sockfd);
                    }

                } else if (events[i].data.fd == udp_listenerfd) {
                    // handle UDP message
                    char buf[UDP_MES_SIZE];
                    memset(buf, 0, UDP_MES_SIZE);
                    struct sockaddr_in udp_client_addr;
                    socklen_t addrlen = sizeof(udp_client_addr);

                    int rc = recvfrom(udp_listenerfd, &buf, sizeof(buf) - 1, 0, 
                            (struct sockaddr *)&udp_client_addr, &addrlen);

                    if (rc < 0) {
                        fprintf(stderr, "Error with recieving data\n");
                        exit(1);
                    }

                    struct udp_message *mes = create_udp_message(buf);

                    // attach sender info
                    strcpy(mes->ip ,inet_ntoa(udp_client_addr.sin_addr));
                    mes->port = ntohs(udp_client_addr.sin_port);

                    // if no wildcards in topic
                    if (strchr(mes->topic, '/') == NULL) {
                        for (int i = 0; i < subscribers_topics->size; i++) {
                            struct pairStruct* pair = &subscribers_topics->data[i];
                            if (is_subscribed_no_wildcard(pair, mes->topic) == 1) {
                                struct pairStruct* id_socket_pair = map_get(subscribers_map, pair->key);
                                if (id_socket_pair != NULL) {
                                    send_all(*((int *)id_socket_pair->value), mes, sizeof(struct udp_message));
                                }
                            }
                        }
                    } else { // wildcard topics
                        for (int i = 0; i < subscribers_topics->size; i++) {
                            struct pairStruct* pair = &subscribers_topics->data[i];
                            if (is_subscribed_wildcard(pair, mes->topic) == 1) {
                                struct pairStruct* id_socket_pair = map_get(subscribers_map, pair->key);
                                if (id_socket_pair != NULL) {
                                    send_all(*((int *)id_socket_pair->value), mes, sizeof(struct udp_message));
                                }
                            }
                        }
                    }

                } else if (events[i].data.fd == STDIN_FILENO) {
                    // check for "exit" command from user
                    char buf[10];
                    if (fgets(buf, sizeof(buf), stdin)) {
                        buf[strlen(buf) - 1] = '\0';
                        if (strcmp(buf, "exit") == 0) {
                            close(epollfd);
                            map_free(subscribers_map); 
                            map_free(subscribers_topics);
                            return;
                        }
                    } else {
                        fprintf(stderr, "Error with inputing info\n");
                        exit(1);
                    }
                } else {
                    // handle messages from already connected TCP clients
                    struct tcp_message recv_mes;
                    recv_all(events[i].data.fd, &recv_mes, sizeof(recv_mes));

                    if (strcmp(recv_mes.command, "subscribe") == 0) {
                        map_add_string_to_array(subscribers_topics, recv_mes.id, recv_mes.message);
                    } else if (strcmp(recv_mes.command, "unsubscribe") == 0) {
                        remove_topic(subscribers_topics, recv_mes.id, (char *)recv_mes.message);
                    } else if (strstr(recv_mes.command, "exit") != 0) {
                        // client wants to disconnect
                        map_remove(subscribers_map, recv_mes.id);
                        rc = epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                        if (rc < 0) {
                            fprintf(stderr, "Error with deleting socket\n");
                            exit(1);
                        }
                        close(events[i].data.fd);
                        printf("Client %s disconnected.\n", recv_mes.id);
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Not enough arguments!\n");
        exit(1);
    }

    // turn off stdout buffering
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    int port = atoi(argv[1]);

    // set up TCP and UDP listeners
    int tcp_listener = create_tcp_listener(port);
    int udp_listener = create_udp_listener(port);

    // start server
    run_server(tcp_listener, udp_listener);
    
    // cleanup
    close(tcp_listener);
    close(udp_listener);

    return 0;
}
