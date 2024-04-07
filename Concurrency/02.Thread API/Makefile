CFLAGS := -Wall -Werror -I../include -pthread

all: thread_create thread_create_with_return_args thread_create_simple_args \
	main-race main-deadlock main-deadlock-global main-signal main-signal-cv

clean:
	rm -f thread_create thread_create_with_return_args thread_create_simple_args \
	 main-race main-deadlock main-deadlock-global main-signal main-signal-cv

thread_create:
	gcc -o thread_create thread_create.c $(CFLAGS)

thread_create_with_return_args:
	gcc -o thread_create_with_return_args thread_create_with_return_args.c $(CFLAGS)

thread_create_simple_args:
	gcc -o thread_create_simple_args thread_create_simple_args.c $(CFLAGS)

main-race:
	gcc -o main-race main-race.c $(CFLAGS)

main-deadlock:
	gcc -o main-deadlock main-deadlock.c $(CFLAGS)

main-deadlock-global:
	gcc -o main-deadlock-global main-deadlock-global.c $(CFLAGS)

main-singal:
	gcc -o main-signal main-signal.c $(CFLAGS)

main-signal-cv:
	gcc -o main-signal-cv main-signal-cv.c $(CFLAGS)