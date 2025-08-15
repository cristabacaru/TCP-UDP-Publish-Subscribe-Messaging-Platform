#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

// helper function that makes sure we receive all the bytes we expect
int recv_all(int sockfd, void *buffer, size_t len) {
    size_t bytes_received = 0;
    size_t bytes_remaining = len;
    char *buff = buffer;

    while(bytes_remaining) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return bytes_received;

        size_t currb_recv = recv(sockfd, buff + bytes_received, bytes_remaining, 0);
        if (currb_recv < 0) {
            printf("error with recv\n");
            exit(1);
        } else if (currb_recv == 0) {
            break;  // connection closed
        }

        bytes_remaining -= currb_recv;
        bytes_received += currb_recv;
    }

    return (int)bytes_received;
}

// helper function that makes sure we send all the bytes we want
int send_all(int sockfd, void *buffer, size_t len) {
    size_t bytes_sent = 0;
    size_t bytes_remaining = len;
    char *buff = buffer;

    while(bytes_remaining) {
        size_t currb_sent = send(sockfd, buff + bytes_sent, bytes_remaining, 0);
        if (currb_sent < 0) {
            printf("error with send\n");
            exit(1);
        } else if (currb_sent == 0) {
            break;  // connection closed
        }

        bytes_remaining -= currb_sent;
        bytes_sent += currb_sent;
    }

    return (int)bytes_sent;
}

// function that checks if a topic matches a subscription path (with wildcards)
bool find_match(char full_topic[51], char path[51]) {
    // make local copies so we can modify them safely
    char topic[51] = "";
    strcpy(topic, full_topic);
    char path_copy[51] = "";
    strcpy(path_copy, path);

    // split full_topic into parts by "/"
    char full_topic_words[51][51];
    int ftw_size = 0;
    char *p = strtok(topic, "/");
    while (p) {
        strcpy(full_topic_words[ftw_size++], p);
        p = strtok(NULL, "/");
    }

    // split path into parts by "/"
    char path_words[51][51];
    int pw_size = 0;
    p = strtok(path_copy, "/");
    while(p) {
        strcpy(path_words[pw_size++], p);
        p = strtok(NULL, "/");
    }

    int topic_index = 0, path_index = 0;

    while (path_index < pw_size) {
        if (topic_index >= ftw_size) {
            return false;  // ran out of topic to compare
        }

        if (strcmp(full_topic_words[topic_index], path_words[path_index]) == 0) {
            // normal word match
            topic_index++;
            path_index++;
            continue;
        }

        if (strcmp(path_words[path_index], "+") == 0) {
            // "+" matches any single word
            topic_index++;
            path_index++;
            continue;
        }

        if (strcmp(path_words[path_index], "*") == 0) {
            // "*" matches everything until the rest of the path
            if (path_index == pw_size - 1) {
                // if "*" is last, it matches all remaining topic parts
                return true;
            }

            // otherwise, find the next match after the "*"
            path_index++;
            topic_index++;

            while (topic_index < ftw_size) {
                if (strcmp(path_words[path_index], full_topic_words[topic_index]) == 0) {
                    // found matching word, continue from here
                    topic_index++;
                    path_index++;
                    break;
                }
                topic_index++;
                if (topic_index == ftw_size)
                    return false;  // couldn't match after "*"
            }
            continue;
        }

        // no match and not a wildcard = mismatch
        return false;
    }

    return (topic_index == ftw_size);  // both must end at the same time
}
