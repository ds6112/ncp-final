#Sample Makefile. You can make changes to this file according to your need
# The executable must be named proxy

CC = gcc
CFLAGS = -Wall -g 
LDFLAGS = -lpthread

OBJS = server.o

all: server

server: $(OBJS)

server.o: server.c
	$(CC) $(CFLAGS) -c server.c
clean:
	rm -f *~ *.o server

