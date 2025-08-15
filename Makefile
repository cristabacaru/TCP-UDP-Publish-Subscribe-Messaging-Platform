CFLAGS = -Wall -g -Werror -Wno-error=unused-variable -std=c11
CC = gcc

all: server subscriber

server: server.c map.c common.c

subscriber: subscriber.c map.c common.c -lm

clean:
	rm -f server subscriber *.o *.dSYM