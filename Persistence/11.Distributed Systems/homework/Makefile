CC = g++
CFLAGS = -Wall -Wextra -std=c++11
LDFLAGS =

server: server.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

client: client.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

make all: server client

clean:
	rm -f server client
