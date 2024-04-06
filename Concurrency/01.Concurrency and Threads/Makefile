CFLAGS := -Wall -Werror -I../include -pthread

all: create_thread shared_data

clean:
	rm -f create_thread shared_data

create_thread: create_thread.c
	gcc -o create_thread create_thread.c $(CFLAGS)

shared_data: shared_data.c
	gcc -o shared_data shared_data.c $(CFLAGS)